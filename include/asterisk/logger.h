/*
 * Asterisk -- An open source telephony toolkit.
 *
 * Copyright (C) 1999 - 2005, Digium, Inc.
 *
 * Mark Spencer <markster@digium.com>
 *
 * See http://www.asterisk.org for more information about
 * the Asterisk project. Please do not directly contact
 * any of the maintainers of this project for assistance;
 * the project provides a web site, mailing lists and IRC
 * channels for your use.
 *
 * This program is free software, distributed under the terms of
 * the GNU General Public License Version 2. See the LICENSE file
 * at the top of the source tree.
 */

/*!
  \file logger.h
  \brief Support for logging to various files, console and syslog
	Configuration in file logger.conf
*/

#ifndef _ASTERISK_LOGGER_H
#define _ASTERISK_LOGGER_H

#include "asterisk/options.h"	/* need option_debug */

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#define EVENTLOG "event_log"
#define	QUEUELOG	"queue_log"
//<VITEL>
#define VITELOG "vitel_log"
//</VITEL>


#define DEBUG_M(a) { \
	a; \
}

#define VERBOSE_PREFIX_1 " "
#define VERBOSE_PREFIX_2 "  == "
#define VERBOSE_PREFIX_3 "    -- "
#define VERBOSE_PREFIX_4 "       > "

#define AST_CALLID_BUFFER_LENGTH 13

/*! \brief Used for sending a log message
	This is the standard logger function.  Probably the only way you will invoke it would be something like this:
	ast_log(AST_LOG_WHATEVER, "Problem with the %s Captain.  We should get some more.  Will %d be enough?\n", "flux capacitor", 10);
	where WHATEVER is one of ERROR, DEBUG, EVENT, NOTICE, or WARNING depending
	on which log you wish to output to. These are implemented as macros, that
	will provide the function with the needed arguments.

 	\param level	Type of log event
	\param file	Will be provided by the AST_LOG_* macro
	\param line	Will be provided by the AST_LOG_* macro
	\param function	Will be provided by the AST_LOG_* macro
	\param fmt	This is what is important.  The format is the same as your favorite breed of printf.  You know how that works, right? :-)
 */

void ast_log(int level, const char *file, int line, const char *function, const char *fmt, ...)
	__attribute__((format(printf, 5, 6)));

/* XXX needs documentation */
struct ast_callid;

/*! \brief Used for sending a log message with a known call_id
	This is a modified logger function which is functionally identical to the above logger function,
	it just include a call_id argument as well. If NULL is specified here, no attempt will be made to
	join the log message with a call_id.

	\param level	Type of log event
	\param file	Will be provided by the AST_LOG_* macro
	\param line	Will be provided by the AST_LOG_* macro
	\param function	Will be provided by the AST_LOG_* macro
	\param callid	This is the ast_callid that is associated with the log message. May be NULL.
	\param fmt	This is what is important.  The format is the same as your favorite breed of printf.  You know how that works, right? :-)
*/
void ast_log_callid(int level, const char *file, int line, const char *function, struct ast_callid *callid, const char *fmt, ...)
	__attribute__((format(printf, 6, 7)));

void ast_backtrace(void);

/*! \brief Reload logger without rotating log files */
int logger_reload(void);

void __attribute__((format(printf, 5, 6))) ast_queue_log(const char *queuename, const char *callid, const char *agent, const char *event, const char *fmt, ...);

//<VITEL>
void __attribute__((format(printf, 8, 9))) ast_vitel_log(const char *funcName, const char *callid, const char *src, const char *dst, const char *exten, const char *event, const char *data, const char *fmt, ...);
//</VITEL>

/*! Send a verbose message (based on verbose level)
 *	\brief This works like ast_log, but prints verbose messages to the console depending on verbosity level set.
 *	ast_verbose(VERBOSE_PREFIX_3 "Whatever %s is happening\n", "nothing");
 *	This will print the message to the console if the verbose level is set to a level >= 3
 *	Note the absence of a comma after the VERBOSE_PREFIX_3.  This is important.
 *	VERBOSE_PREFIX_1 through VERBOSE_PREFIX_4 are defined.
 *  \version 11 added level parameter
 */
void __attribute__((format(printf, 5, 6))) __ast_verbose(const char *file, int line, const char *func, int level, const char *fmt, ...);

/*! Send a verbose message (based on verbose level) with deliberately specified callid
 *  \brief just like __ast_verbose, only __ast_verbose_callid allows you to specify which callid is being used
 *  for the log without needing to bind it to a thread. NULL is a valid argument for this function and will
 *  allow you to specify that a log will never display a call id even when there is a call id bound to the
 *  thread.
 */
void __attribute__((format(printf, 6, 7))) __ast_verbose_callid(const char *file, int line, const char *func, int level, struct ast_callid *callid, const char *fmt, ...);

#define ast_verbose(...) __ast_verbose(__FILE__, __LINE__, __PRETTY_FUNCTION__, -1, __VA_ARGS__)
#define ast_verbose_callid(callid, ...) __ast_verbose_callid(__FILE__, __LINE__, __PRETTY_FUNCTION__, -1, callid, __VA_ARGS__)

void __attribute__((format(printf, 6, 0))) __ast_verbose_ap(const char *file, int line, const char *func, int level, struct ast_callid *callid, const char *fmt, va_list ap);

void __attribute__((format(printf, 2, 3))) ast_child_verbose(int level, const char *fmt, ...);

int ast_register_verbose(void (*verboser)(const char *string)) attribute_warn_unused_result;
int ast_unregister_verbose(void (*verboser)(const char *string)) attribute_warn_unused_result;

void ast_console_puts(const char *string);

/*!
 * \brief log the string to the console, and all attached
 * console clients
 * \version 1.6.1 added level parameter
 */
void ast_console_puts_mutable(const char *string, int level);
void ast_console_toggle_mute(int fd, int silent);

/*!
 * \brief enables or disables logging of a specified level to the console
 * fd specifies the index of the console receiving the level change
 * level specifies the index of the logging level being toggled
 * state indicates whether logging will be on or off (0 for off, 1 for on)
 */
void ast_console_toggle_loglevel(int fd, int level, int state);

/* Note: The AST_LOG_* macros below are the same as
 * the LOG_* macros and are intended to eventually replace
 * the LOG_* macros to avoid name collisions with the syslog(3)
 * log levels. However, please do NOT remove
 * the LOG_* macros from the source since these may be still
 * needed for third-party modules
 */

#define _A_ __FILE__, __LINE__, __PRETTY_FUNCTION__

#ifdef LOG_DEBUG
#undef LOG_DEBUG
#endif
#define __LOG_DEBUG    0
#define LOG_DEBUG      __LOG_DEBUG, _A_

#ifdef AST_LOG_DEBUG
#undef AST_LOG_DEBUG
#endif
#define AST_LOG_DEBUG      __LOG_DEBUG, _A_

#ifdef LOG_NOTICE
#undef LOG_NOTICE
#endif
#define __LOG_NOTICE   2
#define LOG_NOTICE     __LOG_NOTICE, _A_

#ifdef AST_LOG_NOTICE
#undef AST_LOG_NOTICE
#endif
#define AST_LOG_NOTICE     __LOG_NOTICE, _A_

#ifdef LOG_WARNING
#undef LOG_WARNING
#endif
#define __LOG_WARNING  3
#define LOG_WARNING    __LOG_WARNING, _A_

#ifdef AST_LOG_WARNING
#undef AST_LOG_WARNING
#endif
#define AST_LOG_WARNING    __LOG_WARNING, _A_

#ifdef LOG_ERROR
#undef LOG_ERROR
#endif
#define __LOG_ERROR    4
#define LOG_ERROR      __LOG_ERROR, _A_

#ifdef AST_LOG_ERROR
#undef AST_LOG_ERROR
#endif
#define AST_LOG_ERROR      __LOG_ERROR, _A_

#ifdef LOG_VERBOSE
#undef LOG_VERBOSE
#endif
#define __LOG_VERBOSE  5
#define LOG_VERBOSE    __LOG_VERBOSE, _A_

#ifdef AST_LOG_VERBOSE
#undef AST_LOG_VERBOSE
#endif
#define AST_LOG_VERBOSE    __LOG_VERBOSE, _A_

#ifdef LOG_DTMF
#undef LOG_DTMF
#endif
#define __LOG_DTMF  6
#define LOG_DTMF    __LOG_DTMF, _A_

#ifdef AST_LOG_DTMF
#undef AST_LOG_DTMF
#endif
#define AST_LOG_DTMF    __LOG_DTMF, _A_

#define NUMLOGLEVELS 32

/*!
 * \brief Get the debug level for a module
 * \param module the name of module
 * \return the debug level
 */
unsigned int ast_debug_get_by_module(const char *module);

/*!
 * \brief Get the verbose level for a module
 * \param module the name of module
 * \return the verbose level
 */
unsigned int ast_verbose_get_by_module(const char *module);

/*!
 * \brief Register a new logger level
 * \param name The name of the level to be registered
 * \retval -1 if an error occurs
 * \retval non-zero level to be used with ast_log for sending messages to this level
 * \since 1.8
 */
int ast_logger_register_level(const char *name);

/*!
 * \brief Unregister a previously registered logger level
 * \param name The name of the level to be unregistered
 * \return nothing
 * \since 1.8
 */
void ast_logger_unregister_level(const char *name);

/*!
 * \brief factory function to create a new uniquely identifying callid.
 *
 * \retval ast_callid struct pointer containing the call id
 *
 * \note The newly created callid will be referenced upon creation and this function should be
 * paired with a call to ast_callid_unref()
 */
struct ast_callid *ast_create_callid(void);

/*!
 * \brief extracts the callerid from the thread
 *
 * \retval ast_callid reference to call_id related to the thread
 * \retval NULL if no call_id is present in the thread
 *
 * This reference must be unreffed before it loses scope to prevent memory leaks.
 */
struct ast_callid *ast_read_threadstorage_callid(void);

/*!
 * \brief Increase callid reference count
 *
 * \param c the ast_callid
 *
 * \retval c always
 */
#define ast_callid_ref(c) ({ ao2_ref(c, +1); (c); })

/*!
 * \brief Decrease callid reference count
 *
 * \param c the ast_callid
 *
 * \retval NULL always
 */
#define ast_callid_unref(c) ({ ao2_ref(c, -1); (NULL); })

/*!
 * \brief Sets what is stored in the thread storage to the given
 *        callid if it does not match what is already there.
 *
 * \retval 0 - success
 * \retval non-zero - failure
 */
int ast_callid_threadassoc_change(struct ast_callid *callid);

/*!
 * \brief Adds a known callid to thread storage of the calling thread
 *
 * \retval 0 - success
 * \retval non-zero - failure
 */
int ast_callid_threadassoc_add(struct ast_callid *callid);

/*!
 * \brief Removes callid from thread storage of the calling thread
 *
 * \retval 0 - success
 * \retval non-zero - failure
 */
int ast_callid_threadassoc_remove(void);

/*!
 * \brief Checks thread storage for a callid and stores a reference if it exists.
 *        If not, then a new one will be created, bound to the thread, and a reference
 *        to it will be stored.
 *
 * \param callid pointer to struct pointer used to store the referenced callid
 * \retval 0 - callid was found
 * \retval 1 - callid was created
 * \retval -1 - the function failed somehow (presumably memory problems)
 */
int ast_callid_threadstorage_auto(struct ast_callid **callid);

/*!
 * \brief Use in conjunction with ast_callid_threadstorage_auto. Cleans up the
 *        references and if the callid was created by threadstorage_auto, unbinds
 *        the callid from the threadstorage
 * \param callid The callid set by ast_callid_threadstorage_auto
 * \param callid_created The integer returned through ast_callid_threadstorage_auto
 */
void ast_callid_threadstorage_auto_clean(struct ast_callid *callid, int callid_created);

/*!
 * \brief copy a string representation of the callid into a target string
 *
 * \param buffer destination of callid string (should be able to store 13 characters or more)
 * \param buffer_size maximum writable length of the string (Less than 13 will result in truncation)
 * \param callid Callid for which string is being requested
 */
void ast_callid_strnprint(char *buffer, size_t buffer_size, struct ast_callid *callid);

/*!
 * \brief Send a log message to a dynamically registered log level
 * \param level The log level to send the message to
 *
 * Like ast_log, the log message may include printf-style formats, and
 * the data for these must be provided as additional parameters after
 * the log message.
 *
 * \return nothing
 * \since 1.8
 */

#define ast_log_dynamic_level(level, ...) ast_log(level, __FILE__, __LINE__, __PRETTY_FUNCTION__, __VA_ARGS__)

/*!
 * \brief Log a DEBUG message
 * \param level The minimum value of option_debug for this message
 *        to get logged
 */
#define ast_debug(level, ...) do {       \
	if (option_debug >= (level) || (ast_opt_dbg_module && ast_debug_get_by_module(AST_MODULE) >= (level)) ) \
		ast_log(AST_LOG_DEBUG, __VA_ARGS__); \
} while (0)

#define ast_verb(level, ...) __ast_verbose(__FILE__, __LINE__, __PRETTY_FUNCTION__, level, __VA_ARGS__)
#define ast_verb_callid(level, callid, ...) __ast_verbose_callid(__FILE__, __LINE__, __PRETTY_FUNCTION__, level, callid, __VA_ARGS__)

#ifndef _LOGGER_BACKTRACE_H
#define _LOGGER_BACKTRACE_H
#ifdef HAVE_BKTR
#define AST_MAX_BT_FRAMES 32
/* \brief
 *
 * A structure to hold backtrace information. This structure provides an easy means to
 * store backtrace information or pass backtraces to other functions.
 */
struct ast_bt {
	/*! The addresses of the stack frames. This is filled in by calling the glibc backtrace() function */
	void *addresses[AST_MAX_BT_FRAMES];
	/*! The number of stack frames in the backtrace */
	int num_frames;
	/*! Tells if the ast_bt structure was dynamically allocated */
	unsigned int alloced:1;
};

/* \brief
 * Allocates memory for an ast_bt and stores addresses and symbols.
 *
 * \return Returns NULL on failure, or the allocated ast_bt on success
 * \since 1.6.1
 */
struct ast_bt *ast_bt_create(void);

/* \brief
 * Fill an allocated ast_bt with addresses
 *
 * \retval 0 Success
 * \retval -1 Failure
 * \since 1.6.1
 */
int ast_bt_get_addresses(struct ast_bt *bt);

/* \brief
 *
 * Free dynamically allocated portions of an ast_bt
 *
 * \retval NULL.
 * \since 1.6.1
 */
void *ast_bt_destroy(struct ast_bt *bt);

/* \brief Retrieve symbols for a set of backtrace addresses
 *
 * \param addresses A list of addresses, such as the ->addresses structure element of struct ast_bt.
 * \param num_frames Number of addresses in the addresses list
 * \retval NULL Unable to allocate memory
 * \return List of strings
 * \since 1.6.2.16
 */
char **ast_bt_get_symbols(void **addresses, size_t num_frames);

#endif /* HAVE_BKTR */
#endif /* _LOGGER_BACKTRACE_H */

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif /* _ASTERISK_LOGGER_H */
