/*
 * ptype.h
 * Types are a syntatical template which parameters reference.
 */

#ifndef _clish_ptype_h
#define _clish_ptype_h

typedef struct clish_ptype_s clish_ptype_t;

#include "lub/types.h"
#include "clish/macros.h"
#include "lub/bintree.h"
#include "lub/argv.h"
#include "clish/action.h"

#include <stddef.h>

/* The means by which the pattern is interpreted and validated. */
typedef enum {
	/* [default] - A POSIX regular expression. */
	CLISH_PTYPE_METHOD_REGEXP,
	/* A numeric definition "min..max" signed and unsigned versions */
	CLISH_PTYPE_METHOD_INTEGER,
	CLISH_PTYPE_METHOD_UNSIGNEDINTEGER,
	/* A list of possible values. The syntax of the string is of the form:
	* "valueOne(ONE) valueTwo(TWO) valueThree(THREE)" where the text before
	* the parethesis defines the syntax that the user must use, and the
	* value within the parenthesis is the result expanded as a parameter value.
	*/
	CLISH_PTYPE_METHOD_SELECT,
	/* User-defined code in ACTION */
	CLISH_PTYPE_METHOD_CODE,
	/* Used to detect errors */
	CLISH_PTYPE_METHOD_MAX

} clish_ptype_method_e;

/* This defines the pre processing which is to be performed before a string is validated. */
typedef enum {
	/* [default] - do nothing */
	CLISH_PTYPE_PRE_NONE,
	/* before validation convert to uppercase. */
	CLISH_PTYPE_PRE_TOUPPER,
	/* before validation convert to lowercase. */
	CLISH_PTYPE_PRE_TOLOWER,
	/* Used to detect errors */
	CLISH_PTYPE_PRE_MAX
} clish_ptype_preprocess_e;

int clish_ptype_compare(const void *first, const void *second);
const char *clish_ptype__get_method_name(clish_ptype_method_e method);
clish_ptype_method_e clish_ptype_method_resolve(const char *method_name);
const char *clish_ptype__get_preprocess_name(clish_ptype_preprocess_e preprocess);
clish_ptype_preprocess_e clish_ptype_preprocess_resolve(const char *preprocess_name);
clish_ptype_t *clish_ptype_new(const char *name, const char *text,
	const char *pattern, clish_ptype_method_e method,
	clish_ptype_preprocess_e preprocess);

void clish_ptype_free(void *instance);
/**
 * This is the validation method for the specified type.
 * \return
 * - NULL if the validation is negative.
 * - A pointer to a string containing the validated text. NB. this
 *   may not be identical to that passed in. e.g. it may have been
 *   a case-modified "select" or a preprocessed value.
 */
char *clish_ptype_validate(const clish_ptype_t * instance, const char *text);
/**
 * This is the translation method for the specified type. The text is
 * first validated then translated into the form which should be used
 * for variable substitutions in ACTION or VIEW_ID fields.
 * \return
 * - NULL if the validation is negative.
 * - A pointer to a string containing the translated text. NB. this
 *   may not be identical to that passed in. e.g. it may have been
 *   a translated "select" value.
 */
char *clish_ptype_translate(const clish_ptype_t * instance, const char *text);
/**
 * This is used to perform parameter auto-completion
 */
void clish_ptype_word_generator(clish_ptype_t * instance,
	lub_argv_t *matches, const char *text);
void clish_ptype_dump(clish_ptype_t * instance);

_CLISH_GET_STR(ptype, name);
_CLISH_SET_STR_ONCE(ptype, text);
_CLISH_GET_STR(ptype, text);
_CLISH_SET_ONCE(ptype, clish_ptype_preprocess_e, preprocess);
_CLISH_GET_STR(ptype, range);
_CLISH_GET(ptype, clish_action_t *, action);

void clish_ptype__set_pattern(clish_ptype_t * instance,
	const char *pattern, clish_ptype_method_e method);

#endif	/* _clish_ptype_h */
