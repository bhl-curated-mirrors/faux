/*
 * shell_startup.c
 */
#include "private.h"
#include <assert.h>

#include "lub/string.h"

/* Default hooks */
const char* clish_plugin_default_hook[] = {
	NULL,
	"clish_script@clish",
	"clish_hook_access@clish",
	"clish_hook_config@clish",
	"clish_hook_log@clish"
};

/*----------------------------------------------------------- */
int clish_shell_startup(clish_shell_t *this)
{
	const char *banner;
	clish_context_t context;

	if (!this->startup) {
		fprintf(stderr, "Error: Can't get valid STARTUP tag.\n");
		return -1;
	}

	/* Prepare context */
	clish_context_init(&context, this);
	clish_context__set_cmd(&context, this->startup);
	clish_context__set_action(&context,
		clish_command__get_action(this->startup));

	banner = clish_command__get_detail(this->startup);
	if (banner)
		tinyrl_printf(this->tinyrl, "%s\n", banner);

	/* Call log initialize */
	if (clish_shell__get_log(this))
		clish_shell_exec_log(&context, NULL, 0);
	/* Call startup script */
	return clish_shell_execute(&context, NULL);
}

/*----------------------------------------------------------- */
void clish_shell__set_startup_view(clish_shell_t *this, const char *viewname)
{
	clish_view_t *view;

	assert(this);
	assert(this->startup);
	/* Search for the view */
	view = clish_shell_find_view(this, viewname);
	assert(view);
	clish_command__force_viewname(this->startup, viewname);
}

/*----------------------------------------------------------- */
void clish_shell__set_startup_viewid(clish_shell_t *this, const char *viewid)
{
	assert(this);
	assert(this->startup);
	clish_command__force_viewid(this->startup, viewid);
}

/*----------------------------------------------------------- */
void clish_shell__set_default_shebang(clish_shell_t *this, const char *shebang)
{
	assert(this);
	lub_string_free(this->default_shebang);
	this->default_shebang = lub_string_dup(shebang);
}

/*----------------------------------------------------------- */
const char * clish_shell__get_default_shebang(const clish_shell_t *this)
{
	assert(this);
	return this->default_shebang;
}

/*-------------------------------------------------------- */
/* Don't forget:
 *    Global view
 *    hooks
 *    remove unresolved syms list
 */

int clish_shell_prepare(clish_shell_t *this)
{
	clish_command_t *cmd;
	clish_view_t *view;
	clish_nspace_t *nspace;
	lub_bintree_t *view_tree, *cmd_tree;
	lub_list_t *nspace_tree;
	lub_bintree_iterator_t cmd_iter, view_iter;
	lub_list_node_t *nspace_iter;
	clish_hook_access_fn_t *access_fn = NULL;
	int i;

	/* Add default plugin to the list of plugins */
	if (this->default_plugin) {
		clish_plugin_t *plugin;
		plugin = clish_plugin_new("clish");
		lub_list_add(this->plugins, plugin);
		/* Default hooks */
		for (i = 0; i < CLISH_SYM_TYPE_MAX; i++) {
			if (this->hooks_use[i])
				continue;
			if (!clish_plugin_default_hook[i])
				continue;
			clish_sym__set_name(this->hooks[i],
				clish_plugin_default_hook[i]);
		}
	}
	/* Add default syms to unresolved table */
	for (i = 0; i < CLISH_SYM_TYPE_MAX; i++) {
		if (clish_sym__get_name(this->hooks[i]))
			lub_list_add(this->syms, this->hooks[i]);
	}

	/* Load plugins and link symbols */
	if (clish_shell_load_plugins(this) < 0)
		return -1;
	if (clish_shell_link_plugins(this) < 0)
		return -1;

	access_fn = clish_sym__get_func(clish_shell_get_hook(this, CLISH_SYM_TYPE_ACCESS));

	/* Iterate the VIEWs */
	view_tree = &this->view_tree;
	view = lub_bintree_findfirst(view_tree);
	for (lub_bintree_iterator_init(&view_iter, view_tree, view);
		view; view = lub_bintree_iterator_next(&view_iter)) {
		/* Check access rights for the VIEW */
		if (access_fn && clish_view__get_access(view) &&
			access_fn(this, clish_view__get_access(view))) {
			fprintf(stderr, "Warning: Access denied. Remove VIEW %s\n",
				clish_view__get_name(view));
			lub_bintree_remove(view_tree, view);
			clish_view_delete(view);
			continue;
		}

		/* Iterate the NAMESPACEs */
		nspace_tree = clish_view__get_nspace_tree(view);
		nspace_iter = lub_list__get_head(nspace_tree);
		while(nspace_iter) {
			clish_view_t *ref_view;
			lub_list_node_t *old_nspace_iter;
			nspace = (clish_nspace_t *)lub_list_node__get_data(nspace_iter);
			old_nspace_iter = nspace_iter;
			nspace_iter = lub_list_node__get_next(nspace_iter);
			/* Resolve NAMESPACEs and remove unresolved ones */
			ref_view = clish_shell_find_view(this, clish_nspace__get_view_name(nspace));
			if (!ref_view) {
				fprintf(stderr, "Warning: Remove unresolved NAMESPACE %s from %s VIEW\n",
					clish_nspace__get_view_name(nspace), clish_view__get_name(view));
				lub_list_del(nspace_tree, old_nspace_iter);
				lub_list_node_free(old_nspace_iter);
				clish_nspace_delete(nspace);
				continue;
			}
			clish_nspace__set_view(nspace, ref_view);
			/* Check access rights for the NAMESPACE */
			if (access_fn && clish_nspace__get_access(nspace) &&
				access_fn(this, clish_nspace__get_access(nspace))) {
				fprintf(stderr, "Warning: Access denied. Remove NAMESPACE %s from %s VIEW\n",
					clish_nspace__get_view_name(nspace), clish_view__get_name(view));
				lub_list_del(nspace_tree, old_nspace_iter);
				lub_list_node_free(old_nspace_iter);
				clish_nspace_delete(nspace);
				continue;
			}
		}

		/* Iterate the COMMANDs */
		cmd_tree = clish_view__get_command_tree(view);
		cmd = lub_bintree_findfirst(cmd_tree);
		for (lub_bintree_iterator_init(&cmd_iter, cmd_tree, cmd);
			cmd; cmd = lub_bintree_iterator_next(&cmd_iter)) {
			if (!clish_command_alias_to_link(cmd)) {
				fprintf(stderr, CLISH_XML_ERROR_STR"Broken alias %s\n",
					clish_command__get_name(cmd));
				return -1;
			}
		}
	}

	return 0;
}


/*----------------------------------------------------------- */
