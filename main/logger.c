/*
 * Asterisk -- An open source telephony toolkit.
 *
 * Copyright (C) 1999 - 2006, Digium, Inc.
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

/*! \file
 *
 * \brief Asterisk Logger
 *
 * Logging routines
 *
 * \author Mark Spencer <markster@digium.com>
 */

/*** MODULEINFO
	<support_level>core</support_level>
 ***/

#include "asterisk.h"

ASTERISK_FILE_VERSION(__FILE__, "$Revision: 389677 $")

/* When we include logger.h again it will trample on some stuff in syslog.h, but
 * nothing we care about in here. */
#include <syslog.h>

#include "asterisk/_private.h"
#include "asterisk/paths.h"	/* use ast_config_AST_LOG_DIR */
#include "asterisk/logger.h"
#include "asterisk/lock.h"
#include "asterisk/channel.h"
#include "asterisk/config.h"
#include "asterisk/term.h"
#include "asterisk/cli.h"
#include "asterisk/utils.h"
#include "asterisk/manager.h"
#include "asterisk/astobj2.h"
#include "asterisk/threadstorage.h"
#include "asterisk/strings.h"
#include "asterisk/pbx.h"
#include "asterisk/app.h"
#include "asterisk/syslog.h"
#include "asterisk/buildinfo.h"
#include "asterisk/ast_version.h"

#include <signal.h>
#include <time.h>
#include <sys/stat.h>
#include <fcntl.h>
#ifdef HAVE_BKTR
#include <execinfo.h>
#define MAX_BACKTRACE_FRAMES 20
#  if defined(HAVE_DLADDR) && defined(HAVE_BFD) && defined(BETTER_BACKTRACES)
#    include <dlfcn.h>
#    include <bfd.h>
#  endif
#endif

/*** DOCUMENTATION
 ***/

static char dateformat[256] = "%b %e %T";		/* Original Asterisk Format */

static char queue_log_name[256] = QUEUELOG;
//<VITEL>
static char vitel_log_name[256] = VITELOG;
//</VITEL>
static char exec_after_rotate[256] = "";

static int filesize_reload_needed;
static unsigned int global_logmask = 0xFFFF;
static int queuelog_init;
//<VITEL>
static int vitelog_init;
//</VITEL>
static int logger_initialized;
static volatile int next_unique_callid; /* Used to assign unique call_ids to calls */
static int display_callids;
static void unique_callid_cleanup(void *data);

struct ast_callid {
    int call_identifier; /* Numerical value of the call displayed in the logs */
};

AST_THREADSTORAGE_CUSTOM(unique_callid, NULL, unique_callid_cleanup);

static enum rotatestrategy {
	SEQUENTIAL = 1 << 0,     /* Original method - create a new file, in order */
	ROTATE = 1 << 1,         /* Rotate all files, such that the oldest file has the highest suffix */
	TIMESTAMP = 1 << 2,      /* Append the epoch timestamp onto the end of the archived file */
} rotatestrategy = SEQUENTIAL;

static struct {
	unsigned int queue_log:1;
//<VITEL>
	unsigned int vitel_log:1;
//</VITEL>
	unsigned int queue_log_to_file:1;
	unsigned int queue_adaptive_realtime:1;
} logfiles = { 1 };

static char hostname[MAXHOSTNAMELEN];

enum logtypes {
	LOGTYPE_SYSLOG,
	LOGTYPE_FILE,
	LOGTYPE_CONSOLE,
};

struct logchannel {
	/*! What to log to this channel */
	unsigned int logmask;
	/*! If this channel is disabled or not */
	int disabled;
	/*! syslog facility */
	int facility;
	/*! Verbosity level */
	int verbosity;
	/*! Type of log channel */
	enum logtypes type;
	/*! logfile logging file pointer */
	FILE *fileptr;
	/*! Filename */
	char filename[PATH_MAX];
	/*! field for linking to list */
	AST_LIST_ENTRY(logchannel) list;
	/*! Line number from configuration file */
	int lineno;
	/*! Components (levels) from last config load */
	char components[0];
};

static AST_RWLIST_HEAD_STATIC(logchannels, logchannel);

enum logmsgtypes {
	LOGMSG_NORMAL = 0,
	LOGMSG_VERBOSE,
};

struct logmsg {
	enum logmsgtypes type;
	int level;
	int line;
	int lwp;
	struct ast_callid *callid;
	AST_DECLARE_STRING_FIELDS(
		AST_STRING_FIELD(date);
		AST_STRING_FIELD(file);
		AST_STRING_FIELD(function);
		AST_STRING_FIELD(message);
		AST_STRING_FIELD(level_name);
	);
	AST_LIST_ENTRY(logmsg) list;
};

static void logmsg_free(struct logmsg *msg)
{
	if (msg->callid) {
		ast_callid_unref(msg->callid);
	}
	ast_free(msg);
}

static AST_LIST_HEAD_STATIC(logmsgs, logmsg);
static pthread_t logthread = AST_PTHREADT_NULL;
static ast_cond_t logcond;
static int close_logger_thread = 0;

static FILE *qlog;
static FILE *vlog;

/*! \brief Logging channels used in the Asterisk logging system
 *
 * The first 16 levels are reserved for system usage, and the remaining
 * levels are reserved for usage by dynamic levels registered via
 * ast_logger_register_level.
 */

/* Modifications to this array are protected by the rwlock in the
 * logchannels list.
 */

static char *levels[NUMLOGLEVELS] = {
	"DEBUG",
	"---EVENT---",		/* no longer used */
	"NOTICE",
	"WARNING",
	"ERROR",
	"VERBOSE",
	"DTMF",
};

/*! \brief Colors used in the console for logging */
static const int colors[NUMLOGLEVELS] = {
	COLOR_BRGREEN,
	COLOR_BRBLUE,		/* no longer used */
	COLOR_YELLOW,
	COLOR_BRRED,
	COLOR_RED,
	COLOR_GREEN,
	COLOR_BRGREEN,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	COLOR_BRBLUE,
	COLOR_BRBLUE,
	COLOR_BRBLUE,
	COLOR_BRBLUE,
	COLOR_BRBLUE,
	COLOR_BRBLUE,
	COLOR_BRBLUE,
	COLOR_BRBLUE,
	COLOR_BRBLUE,
	COLOR_BRBLUE,
	COLOR_BRBLUE,
	COLOR_BRBLUE,
	COLOR_BRBLUE,
	COLOR_BRBLUE,
	COLOR_BRBLUE,
	COLOR_BRBLUE,
};

AST_THREADSTORAGE(verbose_buf);
#define VERBOSE_BUF_INIT_SIZE   256

AST_THREADSTORAGE(log_buf);
#define LOG_BUF_INIT_SIZE       256

static void logger_queue_init(void);
//<VITEL>
static void logger_vitel_init(void);
//</VITEL>

static unsigned int make_components(const char *s, int lineno, int *verbosity)
{
	char *w;
	unsigned int res = 0;
	char *stringp = ast_strdupa(s);
	unsigned int x;

	*verbosity = 3;

	while ((w = strsep(&stringp, ","))) {
		w = ast_skip_blanks(w);

		if (!strcmp(w, "*")) {
			res = 0xFFFFFFFF;
			break;
		} else if (!strncasecmp(w, "verbose(", 8) && sscanf(w + 8, "%d)", verbosity) == 1) {
			res |= (1 << __LOG_VERBOSE);
			break;
		} else for (x = 0; x < ARRAY_LEN(levels); x++) {
			if (levels[x] && !strcasecmp(w, levels[x])) {
				res |= (1 << x);
				break;
			}
		}
	}

	return res;
}

static struct logchannel *make_logchannel(const char *channel, const char *components, int lineno)
{
	struct logchannel *chan;
	char *facility;
	struct ast_tm tm;
	struct timeval now = ast_tvnow();
	char datestring[256];

	if (ast_strlen_zero(channel) || !(chan = ast_calloc(1, sizeof(*chan) + strlen(components) + 1)))
		return NULL;

	strcpy(chan->components, components);
	chan->lineno = lineno;

	if (!strcasecmp(channel, "console")) {
		chan->type = LOGTYPE_CONSOLE;
	} else if (!strncasecmp(channel, "syslog", 6)) {
		/*
		* syntax is:
		*  syslog.facility => level,level,level
		*/
		facility = strchr(channel, '.');
		if (!facility++ || !facility) {
			facility = "local0";
		}

		chan->facility = ast_syslog_facility(facility);

		if (chan->facility < 0) {
			fprintf(stderr, "Logger Warning: bad syslog facility in logger.conf\n");
			ast_free(chan);
			return NULL;
		}

		chan->type = LOGTYPE_SYSLOG;
		ast_copy_string(chan->filename, channel, sizeof(chan->filename));
		openlog("asterisk", LOG_PID, chan->facility);
	} else {
		if (!ast_strlen_zero(hostname)) {
			snprintf(chan->filename, sizeof(chan->filename), "%s/%s.%s",
				 channel[0] != '/' ? ast_config_AST_LOG_DIR : "", channel, hostname);
		} else {
			snprintf(chan->filename, sizeof(chan->filename), "%s/%s",
				 channel[0] != '/' ? ast_config_AST_LOG_DIR : "", channel);
		}
		if (!(chan->fileptr = fopen(chan->filename, "a"))) {
			/* Can't do real logging here since we're called with a lock
			 * so log to any attached consoles */
			ast_console_puts_mutable("ERROR: Unable to open log file '", __LOG_ERROR);
			ast_console_puts_mutable(chan->filename, __LOG_ERROR);
			ast_console_puts_mutable("': ", __LOG_ERROR);
			ast_console_puts_mutable(strerror(errno), __LOG_ERROR);
			ast_console_puts_mutable("'\n", __LOG_ERROR);
			ast_free(chan);
			return NULL;
		} else {
			/* Create our date/time */
			ast_localtime(&now, &tm, NULL);
			ast_strftime(datestring, sizeof(datestring), dateformat, &tm);

			fprintf(chan->fileptr, "[%s] Asterisk %s built by %s @ %s on a %s running %s on %s\n",
				datestring, ast_get_version(), ast_build_user, ast_build_hostname,
				ast_build_machine, ast_build_os, ast_build_date);
			fflush(chan->fileptr);
		}
		chan->type = LOGTYPE_FILE;
	}
	chan->logmask = make_components(chan->components, lineno, &chan->verbosity);

	return chan;
}

static void init_logger_chain(int locked, const char *altconf)
{
	struct logchannel *chan;
	struct ast_config *cfg;
	struct ast_variable *var;
	const char *s;
	struct ast_flags config_flags = { 0 };

	display_callids = 1;

	if (!(cfg = ast_config_load2(S_OR(altconf, "logger.conf"), "logger", config_flags)) || cfg == CONFIG_STATUS_FILEINVALID) {
		return;
	}

	/* delete our list of log channels */
	if (!locked) {
		AST_RWLIST_WRLOCK(&logchannels);
	}
	while ((chan = AST_RWLIST_REMOVE_HEAD(&logchannels, list))) {
		ast_free(chan);
	}
	global_logmask = 0;
	if (!locked) {
		AST_RWLIST_UNLOCK(&logchannels);
	}

	errno = 0;
	/* close syslog */

	closelog();

	/* If no config file, we're fine, set default options. */
	if (!cfg) {
		if (errno) {
			fprintf(stderr, "Unable to open logger.conf: %s; default settings will be used.\n", strerror(errno));
		} else {
			fprintf(stderr, "Errors detected in logger.conf: see above; default settings will be used.\n");
		}
		if (!(chan = ast_calloc(1, sizeof(*chan)))) {
			return;
		}
		chan->type = LOGTYPE_CONSOLE;
		chan->logmask = __LOG_WARNING | __LOG_NOTICE | __LOG_ERROR;
		if (!locked) {
			AST_RWLIST_WRLOCK(&logchannels);
		}
		AST_RWLIST_INSERT_HEAD(&logchannels, chan, list);
		global_logmask |= chan->logmask;
		if (!locked) {
			AST_RWLIST_UNLOCK(&logchannels);
		}
		return;
	}

	if ((s = ast_variable_retrieve(cfg, "general", "appendhostname"))) {
		if (ast_true(s)) {
			if (gethostname(hostname, sizeof(hostname) - 1)) {
				ast_copy_string(hostname, "unknown", sizeof(hostname));
				fprintf(stderr, "What box has no hostname???\n");
			}
		} else
			hostname[0] = '\0';
	} else
		hostname[0] = '\0';
	if ((s = ast_variable_retrieve(cfg, "general", "display_callids"))) {
		display_callids = ast_true(s);
	}
	if ((s = ast_variable_retrieve(cfg, "general", "dateformat")))
		ast_copy_string(dateformat, s, sizeof(dateformat));
	else
		ast_copy_string(dateformat, "%b %e %T", sizeof(dateformat));
	if ((s = ast_variable_retrieve(cfg, "general", "queue_log"))) {
		logfiles.queue_log = ast_true(s);
	}
//<VITEL>
	if ((s = ast_variable_retrieve(cfg, "general", "vitel_log"))) {
		logfiles.vitel_log = ast_true(s);
	}
//</VITEL>
	if ((s = ast_variable_retrieve(cfg, "general", "queue_log_to_file"))) {
		logfiles.queue_log_to_file = ast_true(s);
	}
	if ((s = ast_variable_retrieve(cfg, "general", "queue_log_name"))) {
		ast_copy_string(queue_log_name, s, sizeof(queue_log_name));
	}
	if ((s = ast_variable_retrieve(cfg, "general", "exec_after_rotate"))) {
		ast_copy_string(exec_after_rotate, s, sizeof(exec_after_rotate));
	}
	if ((s = ast_variable_retrieve(cfg, "general", "rotatestrategy"))) {
		if (strcasecmp(s, "timestamp") == 0) {
			rotatestrategy = TIMESTAMP;
		} else if (strcasecmp(s, "rotate") == 0) {
			rotatestrategy = ROTATE;
		} else if (strcasecmp(s, "sequential") == 0) {
			rotatestrategy = SEQUENTIAL;
		} else {
			fprintf(stderr, "Unknown rotatestrategy: %s\n", s);
		}
	} else {
		if ((s = ast_variable_retrieve(cfg, "general", "rotatetimestamp"))) {
			rotatestrategy = ast_true(s) ? TIMESTAMP : SEQUENTIAL;
			fprintf(stderr, "rotatetimestamp option has been deprecated.  Please use rotatestrategy instead.\n");
		}
	}

	if (!locked) {
		AST_RWLIST_WRLOCK(&logchannels);
	}
	var = ast_variable_browse(cfg, "logfiles");
	for (; var; var = var->next) {
		if (!(chan = make_logchannel(var->name, var->value, var->lineno))) {
			/* Print error message directly to the consoles since the lock is held
			 * and we don't want to unlock with the list partially built */
			ast_console_puts_mutable("ERROR: Unable to create log channel '", __LOG_ERROR);
			ast_console_puts_mutable(var->name, __LOG_ERROR);
			ast_console_puts_mutable("'\n", __LOG_ERROR);
			continue;
		}
		AST_RWLIST_INSERT_HEAD(&logchannels, chan, list);
		global_logmask |= chan->logmask;
	}

	if (qlog) {
		fclose(qlog);
		qlog = NULL;
	}

	if (!locked) {
		AST_RWLIST_UNLOCK(&logchannels);
	}

	ast_config_destroy(cfg);
}

void ast_child_verbose(int level, const char *fmt, ...)
{
	char *msg = NULL, *emsg = NULL, *sptr, *eptr;
	va_list ap, aq;
	int size;

	va_start(ap, fmt);
	va_copy(aq, ap);
	if ((size = vsnprintf(msg, 0, fmt, ap)) < 0) {
		va_end(ap);
		va_end(aq);
		return;
	}
	va_end(ap);

	if (!(msg = ast_malloc(size + 1))) {
		va_end(aq);
		return;
	}

	vsnprintf(msg, size + 1, fmt, aq);
	va_end(aq);

	if (!(emsg = ast_malloc(size * 2 + 1))) {
		ast_free(msg);
		return;
	}

	for (sptr = msg, eptr = emsg; ; sptr++) {
		if (*sptr == '"') {
			*eptr++ = '\\';
		}
		*eptr++ = *sptr;
		if (*sptr == '\0') {
			break;
		}
	}
	ast_free(msg);

	fprintf(stdout, "verbose \"%s\" %d\n", emsg, level);
	fflush(stdout);
	ast_free(emsg);
}

//<VITEL>
void ast_vitel_log(const char *callid, const char *funcName, const char *src, const char *dst, const char *exten, const char *event, const char *data, const char *fmt, ...)
{
	va_list ap;
	char vlog_msg[8192];
	int vlog_len;

	if (!logger_initialized) {
		/* You are too early.  We are not open yet! */
		return;
	}
	if (!vitelog_init) {
		AST_RWLIST_WRLOCK(&logchannels);
		if (!vitelog_init) {
			/*
			 * We have delayed initializing the Vitel logging system so
			 * preloaded realtime modules can get up.  We must initialize
			 * now since someone is trying to log something.
			 */
			logger_vitel_init();
			vitelog_init = 1;
			AST_RWLIST_UNLOCK(&logchannels);
			ast_vitel_log("NONE", "VITALSTART", "NONE", "NONE", "NONE", "NONE", "NONE", "%s", "");
		} else {
			AST_RWLIST_UNLOCK(&logchannels);
		}
	}

	if (vlog) {
		va_start(ap, fmt);
		vlog_len = snprintf(vlog_msg, sizeof(vlog_msg), "%ld|%s|%s|%s|%s|%s|%s|%s", (long)time(NULL), callid, funcName, src, dst, exten, event, data);
		vsnprintf(vlog_msg + vlog_len, sizeof(vlog_msg) - vlog_len, fmt, ap);
		va_end(ap);
		AST_RWLIST_RDLOCK(&logchannels);
		if (vlog) {
			fprintf(vlog, "%s\n", vlog_msg);
			fflush(vlog);
		}
		AST_RWLIST_UNLOCK(&logchannels);
	}
}
	

//</VITEL>

void ast_queue_log(const char *queuename, const char *callid, const char *agent, const char *event, const char *fmt, ...)
{
	va_list ap;
	struct timeval tv;
	struct ast_tm tm;
	char qlog_msg[8192];
	int qlog_len;
	char time_str[30];

	if (!logger_initialized) {
		/* You are too early.  We are not open yet! */
		return;
	}
	if (!queuelog_init) {
		AST_RWLIST_WRLOCK(&logchannels);
		if (!queuelog_init) {
			/*
			 * We have delayed initializing the queue logging system so
			 * preloaded realtime modules can get up.  We must initialize
			 * now since someone is trying to log something.
			 */
			logger_queue_init();
			queuelog_init = 1;
			AST_RWLIST_UNLOCK(&logchannels);
			ast_queue_log("NONE", "NONE", "NONE", "QUEUESTART", "%s", "");
		} else {
			AST_RWLIST_UNLOCK(&logchannels);
		}
	}

	if (ast_check_realtime("queue_log")) {
		tv = ast_tvnow();
		ast_localtime(&tv, &tm, NULL);
		ast_strftime(time_str, sizeof(time_str), "%F %T.%6q", &tm);
		va_start(ap, fmt);
		vsnprintf(qlog_msg, sizeof(qlog_msg), fmt, ap);
		va_end(ap);
		if (logfiles.queue_adaptive_realtime) {
			AST_DECLARE_APP_ARGS(args,
				AST_APP_ARG(data)[5];
			);
			AST_NONSTANDARD_APP_ARGS(args, qlog_msg, '|');
			/* Ensure fields are large enough to receive data */
			ast_realtime_require_field("queue_log",
				"data1", RQ_CHAR, strlen(S_OR(args.data[0], "")),
				"data2", RQ_CHAR, strlen(S_OR(args.data[1], "")),
				"data3", RQ_CHAR, strlen(S_OR(args.data[2], "")),
				"data4", RQ_CHAR, strlen(S_OR(args.data[3], "")),
				"data5", RQ_CHAR, strlen(S_OR(args.data[4], "")),
				SENTINEL);

			/* Store the log */
			ast_store_realtime("queue_log", "time", time_str,
				"callid", callid,
				"queuename", queuename,
				"agent", agent,
				"event", event,
				"data1", S_OR(args.data[0], ""),
				"data2", S_OR(args.data[1], ""),
				"data3", S_OR(args.data[2], ""),
				"data4", S_OR(args.data[3], ""),
				"data5", S_OR(args.data[4], ""),
				SENTINEL);
		} else {
			ast_store_realtime("queue_log", "time", time_str,
				"callid", callid,
				"queuename", queuename,
				"agent", agent,
				"event", event,
				"data", qlog_msg,
				SENTINEL);
		}

		if (!logfiles.queue_log_to_file) {
			return;
		}
	}

	if (qlog) {
		va_start(ap, fmt);
		qlog_len = snprintf(qlog_msg, sizeof(qlog_msg), "%ld|%s|%s|%s|%s|", (long)time(NULL), callid, queuename, agent, event);
		vsnprintf(qlog_msg + qlog_len, sizeof(qlog_msg) - qlog_len, fmt, ap);
		va_end(ap);
		AST_RWLIST_RDLOCK(&logchannels);
		if (qlog) {
			fprintf(qlog, "%s\n", qlog_msg);
			fflush(qlog);
		}
		AST_RWLIST_UNLOCK(&logchannels);
	}
}

static int rotate_file(const char *filename)
{
	char old[PATH_MAX];
	char new[PATH_MAX];
	int x, y, which, found, res = 0, fd;
	char *suffixes[4] = { "", ".gz", ".bz2", ".Z" };

	switch (rotatestrategy) {
	case SEQUENTIAL:
		for (x = 0; ; x++) {
			snprintf(new, sizeof(new), "%s.%d", filename, x);
			fd = open(new, O_RDONLY);
			if (fd > -1)
				close(fd);
			else
				break;
		}
		if (rename(filename, new)) {
			fprintf(stderr, "Unable to rename file '%s' to '%s'\n", filename, new);
			res = -1;
		} else {
			filename = new;
		}
		break;
	case TIMESTAMP:
		snprintf(new, sizeof(new), "%s.%ld", filename, (long)time(NULL));
		if (rename(filename, new)) {
			fprintf(stderr, "Unable to rename file '%s' to '%s'\n", filename, new);
			res = -1;
		} else {
			filename = new;
		}
		break;
	case ROTATE:
		/* Find the next empty slot, including a possible suffix */
		for (x = 0; ; x++) {
			found = 0;
			for (which = 0; which < ARRAY_LEN(suffixes); which++) {
				snprintf(new, sizeof(new), "%s.%d%s", filename, x, suffixes[which]);
				fd = open(new, O_RDONLY);
				if (fd > -1) {
					close(fd);
					found = 1;
					break;
				}
			}
			if (!found) {
				break;
			}
		}

		/* Found an empty slot */
		for (y = x; y > 0; y--) {
			for (which = 0; which < ARRAY_LEN(suffixes); which++) {
				snprintf(old, sizeof(old), "%s.%d%s", filename, y - 1, suffixes[which]);
				fd = open(old, O_RDONLY);
				if (fd > -1) {
					/* Found the right suffix */
					close(fd);
					snprintf(new, sizeof(new), "%s.%d%s", filename, y, suffixes[which]);
					if (rename(old, new)) {
						fprintf(stderr, "Unable to rename file '%s' to '%s'\n", old, new);
						res = -1;
					}
					break;
				}
			}
		}

		/* Finally, rename the current file */
		snprintf(new, sizeof(new), "%s.0", filename);
		if (rename(filename, new)) {
			fprintf(stderr, "Unable to rename file '%s' to '%s'\n", filename, new);
			res = -1;
		} else {
			filename = new;
		}
	}

	if (!ast_strlen_zero(exec_after_rotate)) {
		struct ast_channel *c = ast_dummy_channel_alloc();
		char buf[512];

		pbx_builtin_setvar_helper(c, "filename", filename);
		pbx_substitute_variables_helper(c, exec_after_rotate, buf, sizeof(buf));
		if (c) {
			c = ast_channel_unref(c);
		}
		if (ast_safe_system(buf) == -1) {
			ast_log(LOG_WARNING, "error executing '%s'\n", buf);
		}
	}
	return res;
}

/*!
 * \internal
 * \brief Start the realtime queue logging if configured.
 *
 * \retval TRUE if not to open queue log file.
 */
static int logger_queue_rt_start(void)
{
	if (ast_check_realtime("queue_log")) {
		if (!ast_realtime_require_field("queue_log",
			"time", RQ_DATETIME, 26,
			"data1", RQ_CHAR, 20,
			"data2", RQ_CHAR, 20,
			"data3", RQ_CHAR, 20,
			"data4", RQ_CHAR, 20,
			"data5", RQ_CHAR, 20,
			SENTINEL)) {
			logfiles.queue_adaptive_realtime = 1;
		} else {
			logfiles.queue_adaptive_realtime = 0;
		}

		if (!logfiles.queue_log_to_file) {
			/* Don't open the log file. */
			return 1;
		}
	}
	return 0;
}

/*!
 * \internal
 * \brief Rotate the queue log file and restart.
 *
 * \param queue_rotate Log queue rotation mode.
 *
 * \note Assumes logchannels is write locked on entry.
 *
 * \retval 0 on success.
 * \retval -1 on error.
 */
static int logger_queue_restart(int queue_rotate)
{
	int res = 0;
	char qfname[PATH_MAX];

	if (logger_queue_rt_start()) {
		return res;
	}

	snprintf(qfname, sizeof(qfname), "%s/%s", ast_config_AST_LOG_DIR, queue_log_name);
	if (qlog) {
		/* Just in case it was still open. */
		fclose(qlog);
		qlog = NULL;
	}
	if (queue_rotate) {
		rotate_file(qfname);
	}

	/* Open the log file. */
	qlog = fopen(qfname, "a");
	if (!qlog) {
		ast_log(LOG_ERROR, "Unable to create queue log: %s\n", strerror(errno));
		res = -1;
	}
	return res;
}

//<VITEL>
static int logger_vitel_restart(int vitel_rotate)
{
	int res = 0;
	char vfname[PATH_MAX];

	snprintf(vfname, sizeof(vfname), "%s/%s", ast_config_AST_LOG_DIR, vitel_log_name);
	if (vlog) {
		/* Just in case it was still open. */
		fclose(vlog);
		vlog = NULL;
	}
	if (vitel_rotate) {
		rotate_file(vfname);
	}

	/* Open the log file. */
	vlog = fopen(vfname, "a");
	if (!vlog) {
		ast_log(LOG_ERROR, "Unable to create Vitel log: %s\n", strerror(errno));
		res = -1;
	}
	return res;
}
//</VITEL>


static int reload_logger(int rotate, const char *altconf)
{
	int queue_rotate = rotate;
	int vitel_rotate = rotate;
	struct logchannel *f;
	int res = 0;

	AST_RWLIST_WRLOCK(&logchannels);

	if (qlog) {
		if (rotate < 0) {
			/* Check filesize - this one typically doesn't need an auto-rotate */
			if (ftello(qlog) > 0x40000000) { /* Arbitrarily, 1 GB */
				fclose(qlog);
				qlog = NULL;
			} else {
				queue_rotate = 0;
			}
		} else {
			fclose(qlog);
			qlog = NULL;
		}
	} else {
		queue_rotate = 0;
	}
//<VITEL>
	if (vlog) {
		if (rotate < 0) {
			/* Check filesize - this one typically doesn't need an auto-rotate */
			if (ftello(vlog) > 0x40000000) { /* Arbitrarily, 1 GB */
				fclose(vlog);
				vlog = NULL;
			} else {
				vitel_rotate = 0;
			}
		} else {
			fclose(vlog);
			vlog = NULL;
		}
	} else {
		vitel_rotate = 0;
	}
//</VITEL>

	ast_mkdir(ast_config_AST_LOG_DIR, 0777);

	AST_RWLIST_TRAVERSE(&logchannels, f, list) {
		if (f->disabled) {
			f->disabled = 0;	/* Re-enable logging at reload */
			/*** DOCUMENTATION
				<managerEventInstance>
					<synopsis>Raised when a logging channel is re-enabled after a reload operation.</synopsis>
					<syntax>
						<parameter name="Channel">
							<para>The name of the logging channel.</para>
						</parameter>
					</syntax>
				</managerEventInstance>
			***/
			manager_event(EVENT_FLAG_SYSTEM, "LogChannel", "Channel: %s\r\nEnabled: Yes\r\n", f->filename);
		}
		if (f->fileptr && (f->fileptr != stdout) && (f->fileptr != stderr)) {
			int rotate_this = 0;
			if (ftello(f->fileptr) > 0x40000000) { /* Arbitrarily, 1 GB */
				/* Be more proactive about rotating massive log files */
				rotate_this = 1;
			}
			fclose(f->fileptr);	/* Close file */
			f->fileptr = NULL;
			if (rotate || rotate_this) {
				rotate_file(f->filename);
			}
		}
	}

	filesize_reload_needed = 0;

	init_logger_chain(1 /* locked */, altconf);

	ast_unload_realtime("queue_log");
//<VITEL>
	ast_unload_realtime("vitel_log");
//</VITEL>
	if (logfiles.queue_log) {
		res = logger_queue_restart(queue_rotate);
		AST_RWLIST_UNLOCK(&logchannels);
		ast_queue_log("NONE", "NONE", "NONE", "CONFIGRELOAD", "%s", "");
		ast_verb(1, "Asterisk Queue Logger restarted\n");
//<VITEL>
	} else if (logfiles.vitel_log) {
		res = logger_vitel_restart(vitel_rotate);
		AST_RWLIST_UNLOCK(&logchannels);
		ast_vitel_log("NONE", "CONFIGRELOAD", "NONE", "NONE", "NONE", "NONE", "NONE", "%s", "");
		ast_verb(1, "Asterisk Vitel Logger restarted\n");
//</VITEL>
	} else {
		AST_RWLIST_UNLOCK(&logchannels);
	}

	return res;
}

/*! \brief Reload the logger module without rotating log files (also used from loader.c during
	a full Asterisk reload) */
int logger_reload(void)
{
	if (reload_logger(0, NULL)) {
		return RESULT_FAILURE;
	}
	return RESULT_SUCCESS;
}

static char *handle_logger_reload(struct ast_cli_entry *e, int cmd, struct ast_cli_args *a)
{
	switch (cmd) {
	case CLI_INIT:
		e->command = "logger reload";
		e->usage =
			"Usage: logger reload [<alt-conf>]\n"
			"       Reloads the logger subsystem state.  Use after restarting syslogd(8) if you are using syslog logging.\n";
		return NULL;
	case CLI_GENERATE:
		return NULL;
	}
	if (reload_logger(0, a->argc == 3 ? a->argv[2] : NULL)) {
		ast_cli(a->fd, "Failed to reload the logger\n");
		return CLI_FAILURE;
	}
	return CLI_SUCCESS;
}

static char *handle_logger_rotate(struct ast_cli_entry *e, int cmd, struct ast_cli_args *a)
{
	switch (cmd) {
	case CLI_INIT:
		e->command = "logger rotate";
		e->usage =
			"Usage: logger rotate\n"
			"       Rotates and Reopens the log files.\n";
		return NULL;
	case CLI_GENERATE:
		return NULL;
	}
	if (reload_logger(1, NULL)) {
		ast_cli(a->fd, "Failed to reload the logger and rotate log files\n");
		return CLI_FAILURE;
	}
	return CLI_SUCCESS;
}

static char *handle_logger_set_level(struct ast_cli_entry *e, int cmd, struct ast_cli_args *a)
{
	int x;
	int state;
	int level = -1;

	switch (cmd) {
	case CLI_INIT:
		e->command = "logger set level {DEBUG|NOTICE|WARNING|ERROR|VERBOSE|DTMF} {on|off}";
		e->usage =
			"Usage: logger set level {DEBUG|NOTICE|WARNING|ERROR|VERBOSE|DTMF} {on|off}\n"
			"       Set a specific log level to enabled/disabled for this console.\n";
		return NULL;
	case CLI_GENERATE:
		return NULL;
	}

	if (a->argc < 5)
		return CLI_SHOWUSAGE;

	AST_RWLIST_WRLOCK(&logchannels);

	for (x = 0; x < ARRAY_LEN(levels); x++) {
		if (levels[x] && !strcasecmp(a->argv[3], levels[x])) {
			level = x;
			break;
		}
	}

	AST_RWLIST_UNLOCK(&logchannels);

	state = ast_true(a->argv[4]) ? 1 : 0;

	if (level != -1) {
		ast_console_toggle_loglevel(a->fd, level, state);
		ast_cli(a->fd, "Logger status for '%s' has been set to '%s'.\n", levels[level], state ? "on" : "off");
	} else
		return CLI_SHOWUSAGE;

	return CLI_SUCCESS;
}

/*! \brief CLI command to show logging system configuration */
static char *handle_logger_show_channels(struct ast_cli_entry *e, int cmd, struct ast_cli_args *a)
{
#define FORMATL	"%-35.35s %-8.8s %-9.9s "
	struct logchannel *chan;
	switch (cmd) {
	case CLI_INIT:
		e->command = "logger show channels";
		e->usage =
			"Usage: logger show channels\n"
			"       List configured logger channels.\n";
		return NULL;
	case CLI_GENERATE:
		return NULL;
	}
	ast_cli(a->fd, FORMATL, "Channel", "Type", "Status");
	ast_cli(a->fd, "Configuration\n");
	ast_cli(a->fd, FORMATL, "-------", "----", "------");
	ast_cli(a->fd, "-------------\n");
	AST_RWLIST_RDLOCK(&logchannels);
	AST_RWLIST_TRAVERSE(&logchannels, chan, list) {
		unsigned int level;

		ast_cli(a->fd, FORMATL, chan->filename, chan->type == LOGTYPE_CONSOLE ? "Console" : (chan->type == LOGTYPE_SYSLOG ? "Syslog" : "File"),
			chan->disabled ? "Disabled" : "Enabled");
		ast_cli(a->fd, " - ");
		for (level = 0; level < ARRAY_LEN(levels); level++) {
			if ((chan->logmask & (1 << level)) && levels[level]) {
				ast_cli(a->fd, "%s ", levels[level]);
			}
		}
		ast_cli(a->fd, "\n");
	}
	AST_RWLIST_UNLOCK(&logchannels);
	ast_cli(a->fd, "\n");

	return CLI_SUCCESS;
}

struct verb {
	void (*verboser)(const char *string);
	AST_LIST_ENTRY(verb) list;
};

static AST_RWLIST_HEAD_STATIC(verbosers, verb);

static struct ast_cli_entry cli_logger[] = {
	AST_CLI_DEFINE(handle_logger_show_channels, "List configured log channels"),
	AST_CLI_DEFINE(handle_logger_reload, "Reopens the log files"),
	AST_CLI_DEFINE(handle_logger_rotate, "Rotates and reopens the log files"),
	AST_CLI_DEFINE(handle_logger_set_level, "Enables/Disables a specific logging level for this console")
};

static void _handle_SIGXFSZ(int sig)
{
	/* Indicate need to reload */
	filesize_reload_needed = 1;
}

static struct sigaction handle_SIGXFSZ = {
	.sa_handler = _handle_SIGXFSZ,
	.sa_flags = SA_RESTART,
};

static void ast_log_vsyslog(struct logmsg *msg)
{
	char buf[BUFSIZ];
	int syslog_level = ast_syslog_priority_from_loglevel(msg->level);
	char call_identifier_str[13];

	if (msg->callid) {
		snprintf(call_identifier_str, sizeof(call_identifier_str), "[C-%08x]", msg->callid->call_identifier);
	} else {
		call_identifier_str[0] = '\0';
	}

	if (syslog_level < 0) {
		/* we are locked here, so cannot ast_log() */
		fprintf(stderr, "ast_log_vsyslog called with bogus level: %d\n", msg->level);
		return;
	}

	snprintf(buf, sizeof(buf), "%s[%d]%s: %s:%d in %s: %s",
		 levels[msg->level], msg->lwp, call_identifier_str, msg->file, msg->line, msg->function, msg->message);

	term_strip(buf, buf, strlen(buf) + 1);
	syslog(syslog_level, "%s", buf);
}

/* These gymnastics are due to platforms which designate char as unsigned by
 * default. Level is the negative character -- offset by 1, because \0 is the
 * EOS delimiter. */
#define VERBOSE_MAGIC2LEVEL(x) (((char) -*(signed char *) (x)) - 1)
#define VERBOSE_HASMAGIC(x)	(*(signed char *) (x) < 0)

/*! \brief Print a normal log message to the channels */
static void logger_print_normal(struct logmsg *logmsg)
{
	struct logchannel *chan = NULL;
	char buf[BUFSIZ];
	struct verb *v = NULL;
	int level = 0;

	if (logmsg->level == __LOG_VERBOSE) {
		char *tmpmsg = ast_strdupa(logmsg->message + 1);
		level = VERBOSE_MAGIC2LEVEL(logmsg->message);
		/* Iterate through the list of verbosers and pass them the log message string */
		AST_RWLIST_RDLOCK(&verbosers);
		AST_RWLIST_TRAVERSE(&verbosers, v, list)
			v->verboser(logmsg->message);
		AST_RWLIST_UNLOCK(&verbosers);
		ast_string_field_set(logmsg, message, tmpmsg);
	}

	AST_RWLIST_RDLOCK(&logchannels);

	if (!AST_RWLIST_EMPTY(&logchannels)) {
		AST_RWLIST_TRAVERSE(&logchannels, chan, list) {
			char call_identifier_str[13];

			if (logmsg->callid) {
				snprintf(call_identifier_str, sizeof(call_identifier_str), "[C-%08x]", logmsg->callid->call_identifier);
			} else {
				call_identifier_str[0] = '\0';
			}


			/* If the channel is disabled, then move on to the next one */
			if (chan->disabled) {
				continue;
			}
			if (logmsg->level == __LOG_VERBOSE && level > chan->verbosity) {
				continue;
			}

			/* Check syslog channels */
			if (chan->type == LOGTYPE_SYSLOG && (chan->logmask & (1 << logmsg->level))) {
				ast_log_vsyslog(logmsg);
			/* Console channels */
			} else if (chan->type == LOGTYPE_CONSOLE && (chan->logmask & (1 << logmsg->level))) {
				char linestr[128];
				char tmp1[80], tmp2[80], tmp3[80], tmp4[80];

				/* If the level is verbose, then skip it */
				if (logmsg->level == __LOG_VERBOSE)
					continue;

				/* Turn the numerical line number into a string */
				snprintf(linestr, sizeof(linestr), "%d", logmsg->line);
				/* Build string to print out */
				snprintf(buf, sizeof(buf), "[%s] %s[%d]%s: %s:%s %s: %s",
					 logmsg->date,
					 term_color(tmp1, logmsg->level_name, colors[logmsg->level], 0, sizeof(tmp1)),
					 logmsg->lwp,
					 call_identifier_str,
					 term_color(tmp2, logmsg->file, COLOR_BRWHITE, 0, sizeof(tmp2)),
					 term_color(tmp3, linestr, COLOR_BRWHITE, 0, sizeof(tmp3)),
					 term_color(tmp4, logmsg->function, COLOR_BRWHITE, 0, sizeof(tmp4)),
					 logmsg->message);
				/* Print out */
				ast_console_puts_mutable(buf, logmsg->level);
			/* File channels */
			} else if (chan->type == LOGTYPE_FILE && (chan->logmask & (1 << logmsg->level))) {
				int res = 0;

				/* If no file pointer exists, skip it */
				if (!chan->fileptr) {
					continue;
				}

				/* Print out to the file */
				res = fprintf(chan->fileptr, "[%s] %s[%d]%s %s: %s",
					      logmsg->date, logmsg->level_name, logmsg->lwp, call_identifier_str,
					      logmsg->file, term_strip(buf, logmsg->message, BUFSIZ));
				if (res <= 0 && !ast_strlen_zero(logmsg->message)) {
					fprintf(stderr, "**** Asterisk Logging Error: ***********\n");
					if (errno == ENOMEM || errno == ENOSPC)
						fprintf(stderr, "Asterisk logging error: Out of disk space, can't log to log file %s\n", chan->filename);
					else
						fprintf(stderr, "Logger Warning: Unable to write to log file '%s': %s (disabled)\n", chan->filename, strerror(errno));
					/*** DOCUMENTATION
						<managerEventInstance>
							<synopsis>Raised when a logging channel is disabled.</synopsis>
							<syntax>
								<parameter name="Channel">
									<para>The name of the logging channel.</para>
								</parameter>
							</syntax>
						</managerEventInstance>
					***/
					manager_event(EVENT_FLAG_SYSTEM, "LogChannel", "Channel: %s\r\nEnabled: No\r\nReason: %d - %s\r\n", chan->filename, errno, strerror(errno));
					chan->disabled = 1;
				} else if (res > 0) {
					fflush(chan->fileptr);
				}
			}
		}
	} else if (logmsg->level != __LOG_VERBOSE) {
		fputs(logmsg->message, stdout);
	}

	AST_RWLIST_UNLOCK(&logchannels);

	/* If we need to reload because of the file size, then do so */
	if (filesize_reload_needed) {
		reload_logger(-1, NULL);
		ast_verb(1, "Rotated Logs Per SIGXFSZ (Exceeded file size limit)\n");
	}

	return;
}

/*! \brief Actual logging thread */
static void *logger_thread(void *data)
{
	struct logmsg *next = NULL, *msg = NULL;

	for (;;) {
		/* We lock the message list, and see if any message exists... if not we wait on the condition to be signalled */
		AST_LIST_LOCK(&logmsgs);
		if (AST_LIST_EMPTY(&logmsgs)) {
			if (close_logger_thread) {
				AST_LIST_UNLOCK(&logmsgs);
				break;
			} else {
				ast_cond_wait(&logcond, &logmsgs.lock);
			}
		}
		next = AST_LIST_FIRST(&logmsgs);
		AST_LIST_HEAD_INIT_NOLOCK(&logmsgs);
		AST_LIST_UNLOCK(&logmsgs);

		/* Otherwise go through and process each message in the order added */
		while ((msg = next)) {
			/* Get the next entry now so that we can free our current structure later */
			next = AST_LIST_NEXT(msg, list);

			/* Depending on the type, send it to the proper function */
			logger_print_normal(msg);

			/* Free the data since we are done */
			logmsg_free(msg);
		}
	}

	return NULL;
}

/*!
 * \internal
 * \brief Initialize the logger queue.
 *
 * \note Assumes logchannels is write locked on entry.
 *
 * \return Nothing
 */
static void logger_queue_init(void)
{
	ast_unload_realtime("queue_log");
	if (logfiles.queue_log) {
		char qfname[PATH_MAX];

		if (logger_queue_rt_start()) {
			return;
		}

		/* Open the log file. */
		snprintf(qfname, sizeof(qfname), "%s/%s", ast_config_AST_LOG_DIR,
			queue_log_name);
		if (qlog) {
			/* Just in case it was already open. */
			fclose(qlog);
		}
		qlog = fopen(qfname, "a");
		if (!qlog) {
			ast_log(LOG_ERROR, "Unable to create queue log: %s\n", strerror(errno));
		}
	}
}

//<VITEL>
/*!
 * \internal
 * \brief Initialize the Vitel.
 *
 * \note Assumes logchannels is write locked on entry.
 *
 * \return Nothing
 */
static void logger_vitel_init(void)
{
	ast_unload_realtime("vitel_log");
	if (logfiles.vitel_log) {
		char vfname[PATH_MAX];

		/* Open the log file. */
		snprintf(vfname, sizeof(vfname), "%s/%s", ast_config_AST_LOG_DIR,
			vitel_log_name);
		if (vlog) {
			/* Just in case it was already open. */
			fclose(vlog);
		}
		vlog = fopen(vfname, "a");
		if (!vlog) {
			ast_log(LOG_ERROR, "Unable to create Vitel log: %s\n", strerror(errno));
		}
	}
}
int init_logger(void)
{
	/* auto rotate if sig SIGXFSZ comes a-knockin */
	sigaction(SIGXFSZ, &handle_SIGXFSZ, NULL);

	/* Re-initialize the logmsgs mutex.  The recursive mutex can be accessed prior
 	 * to Asterisk being forked into the background, which can cause the thread
 	 * ID tracked by the underlying pthread mutex to be different than the ID of
 	 * the thread that unlocks the mutex.  Since init_logger is called after the
 	 * fork, it is safe to initialize the mutex here for future accesses.
 	 */
	ast_mutex_destroy(&logmsgs.lock);
	ast_mutex_init(&logmsgs.lock);
	ast_cond_init(&logcond, NULL);

	/* start logger thread */
	if (ast_pthread_create(&logthread, NULL, logger_thread, NULL) < 0) {
		ast_cond_destroy(&logcond);
		return -1;
	}

	/* register the logger cli commands */
	ast_cli_register_multiple(cli_logger, ARRAY_LEN(cli_logger));

	ast_mkdir(ast_config_AST_LOG_DIR, 0777);

	/* create log channels */
	init_logger_chain(0 /* locked */, NULL);
	logger_initialized = 1;

	return 0;
}

void close_logger(void)
{
	struct logchannel *f = NULL;
	struct verb *cur = NULL;

	ast_cli_unregister_multiple(cli_logger, ARRAY_LEN(cli_logger));

	logger_initialized = 0;

	/* Stop logger thread */
	AST_LIST_LOCK(&logmsgs);
	close_logger_thread = 1;
	ast_cond_signal(&logcond);
	AST_LIST_UNLOCK(&logmsgs);

	if (logthread != AST_PTHREADT_NULL)
		pthread_join(logthread, NULL);

	AST_RWLIST_WRLOCK(&verbosers);
	while ((cur = AST_LIST_REMOVE_HEAD(&verbosers, list))) {
		ast_free(cur);
	}
	AST_RWLIST_UNLOCK(&verbosers);

	AST_RWLIST_WRLOCK(&logchannels);

	if (qlog) {
		fclose(qlog);
		qlog = NULL;
	}

	while ((f = AST_LIST_REMOVE_HEAD(&logchannels, list))) {
		if (f->fileptr && (f->fileptr != stdout) && (f->fileptr != stderr)) {
			fclose(f->fileptr);
			f->fileptr = NULL;
		}
		ast_free(f);
	}

	closelog(); /* syslog */

	AST_RWLIST_UNLOCK(&logchannels);
}

void ast_callid_strnprint(char *buffer, size_t buffer_size, struct ast_callid *callid)
{
	snprintf(buffer, buffer_size, "[C-%08x]", callid->call_identifier);
}

struct ast_callid *ast_create_callid(void)
{
	struct ast_callid *call;

	call = ao2_alloc_options(sizeof(struct ast_callid), NULL, AO2_ALLOC_OPT_LOCK_NOLOCK);
	if (!call) {
		ast_log(LOG_ERROR, "Could not allocate callid struct.\n");
		return NULL;
	}

	call->call_identifier = ast_atomic_fetchadd_int(&next_unique_callid, +1);
#ifdef TEST_FRAMEWORK
	ast_debug(3, "CALL_ID [C-%08x] created by thread.\n", call->call_identifier);
#endif
	return call;
}

struct ast_callid *ast_read_threadstorage_callid(void)
{
	struct ast_callid **callid;
	callid = ast_threadstorage_get(&unique_callid, sizeof(struct ast_callid **));
	if (callid && *callid) {
		ast_callid_ref(*callid);
		return *callid;
	}

	return NULL;

}

int ast_callid_threadassoc_change(struct ast_callid *callid)
{
	struct ast_callid **id =
		ast_threadstorage_get(&unique_callid, sizeof(struct ast_callid **));

	if (!id) {
		ast_log(LOG_ERROR, "Failed to allocate thread storage.\n");
		return -1;
	}

	if (*id && (*id != callid)) {
#ifdef TEST_FRAMEWORK
		ast_debug(3, "CALL_ID [C-%08x] being removed from thread.\n", (*id)->call_identifier);
#endif
		*id = ast_callid_unref(*id);
		*id = NULL;
	}

	if (!(*id) && callid) {
		/* callid will be unreffed at thread destruction */
		ast_callid_ref(callid);
		*id = callid;
#ifdef TEST_FRAMEWORK
		ast_debug(3, "CALL_ID [C-%08x] bound to thread.\n", callid->call_identifier);
#endif
	}

	return 0;
}

int ast_callid_threadassoc_add(struct ast_callid *callid)
{
	struct ast_callid **pointing;
	pointing = ast_threadstorage_get(&unique_callid, sizeof(struct ast_callid **));
	if (!(pointing)) {
		ast_log(LOG_ERROR, "Failed to allocate thread storage.\n");
		return -1;
	}

	if (!(*pointing)) {
		/* callid will be unreffed at thread destruction */
		ast_callid_ref(callid);
		*pointing = callid;
#ifdef TEST_FRAMEWORK
		ast_debug(3, "CALL_ID [C-%08x] bound to thread.\n", callid->call_identifier);
#endif
	} else {
		ast_log(LOG_WARNING, "Attempted to ast_callid_threadassoc_add on thread already associated with a callid.\n");
		return 1;
	}

	return 0;
}

int ast_callid_threadassoc_remove()
{
	struct ast_callid **pointing;
	pointing = ast_threadstorage_get(&unique_callid, sizeof(struct ast_callid **));
	if (!(pointing)) {
		ast_log(LOG_ERROR, "Failed to allocate thread storage.\n");
		return -1;
	}

	if (!(*pointing)) {
		ast_log(LOG_ERROR, "Tried to clean callid thread storage with no callid in thread storage.\n");
		return -1;
	} else {
#ifdef TEST_FRAMEWORK
		ast_debug(3, "Call_ID [C-%08x] being removed from thread.\n", (*pointing)->call_identifier);
#endif
		*pointing = ast_callid_unref(*pointing);
		return 0;
	}
}

int ast_callid_threadstorage_auto(struct ast_callid **callid)
{
	struct ast_callid *tmp;

	/* Start by trying to see if a callid is available from thread storage */
	tmp = ast_read_threadstorage_callid();
	if (tmp) {
		*callid = tmp;
		return 0;
	}

	/* If that failed, try to create a new one and bind it. */
	tmp = ast_create_callid();
	if (tmp) {
		ast_callid_threadassoc_add(tmp);
		*callid = tmp;
		return 1;
	}

	/* If neither worked, then something must have gone wrong. */
	return -1;
}

void ast_callid_threadstorage_auto_clean(struct ast_callid *callid, int callid_created)
{
	if (callid) {
		/* If the callid was created rather than simply grabbed from the thread storage, we need to unbind here. */
		if (callid_created == 1) {
			ast_callid_threadassoc_remove();
		}
		callid = ast_callid_unref(callid);
	}
}

/*!
 * \internal
 * \brief thread storage cleanup function for unique_callid
 */
static void unique_callid_cleanup(void *data)
{
	struct ast_callid **callid = data;

	if (*callid) {
		ast_callid_unref(*callid);
	}

	ast_free(data);
}

/*!
 * \brief send log messages to syslog and/or the console
 */
static void __attribute__((format(printf, 6, 0))) ast_log_full(int level, const char *file, int line, const char *function, struct ast_callid *callid, const char *fmt, va_list ap)
{
	struct logmsg *logmsg = NULL;
	struct ast_str *buf = NULL;
	struct ast_tm tm;
	struct timeval now = ast_tvnow();
	int res = 0;
	char datestring[256];

	if (!(buf = ast_str_thread_get(&log_buf, LOG_BUF_INIT_SIZE)))
		return;

	if (level != __LOG_VERBOSE && AST_RWLIST_EMPTY(&logchannels)) {
		/*
		 * we don't have the logger chain configured yet,
		 * so just log to stdout
		 */
		int result;
		result = ast_str_set_va(&buf, BUFSIZ, fmt, ap); /* XXX BUFSIZ ? */
		if (result != AST_DYNSTR_BUILD_FAILED) {
			term_filter_escapes(ast_str_buffer(buf));
			fputs(ast_str_buffer(buf), stdout);
		}
		return;
	}

	/* Ignore anything that never gets logged anywhere */
	if (level != __LOG_VERBOSE && !(global_logmask & (1 << level)))
		return;

	/* Build string */
	res = ast_str_set_va(&buf, BUFSIZ, fmt, ap);

	/* If the build failed, then abort and free this structure */
	if (res == AST_DYNSTR_BUILD_FAILED)
		return;

	/* Create a new logging message */
	if (!(logmsg = ast_calloc_with_stringfields(1, struct logmsg, res + 128)))
		return;

	/* Copy string over */
	ast_string_field_set(logmsg, message, ast_str_buffer(buf));

	/* Set type */
	if (level == __LOG_VERBOSE) {
		logmsg->type = LOGMSG_VERBOSE;
	} else {
		logmsg->type = LOGMSG_NORMAL;
	}

	if (display_callids && callid) {
		logmsg->callid = ast_callid_ref(callid);
		/* callid will be unreffed at logmsg destruction */
	}

	/* Create our date/time */
	ast_localtime(&now, &tm, NULL);
	ast_strftime(datestring, sizeof(datestring), dateformat, &tm);
	ast_string_field_set(logmsg, date, datestring);

	/* Copy over data */
	logmsg->level = level;
	logmsg->line = line;
	ast_string_field_set(logmsg, level_name, levels[level]);
	ast_string_field_set(logmsg, file, file);
	ast_string_field_set(logmsg, function, function);
	logmsg->lwp = ast_get_tid();

	/* If the logger thread is active, append it to the tail end of the list - otherwise skip that step */
	if (logthread != AST_PTHREADT_NULL) {
		AST_LIST_LOCK(&logmsgs);
		if (close_logger_thread) {
			/* Logger is either closing or closed.  We cannot log this message. */
			logmsg_free(logmsg);
		} else {
			AST_LIST_INSERT_TAIL(&logmsgs, logmsg, list);
			ast_cond_signal(&logcond);
		}
		AST_LIST_UNLOCK(&logmsgs);
	} else {
		logger_print_normal(logmsg);
		logmsg_free(logmsg);
	}
}

void ast_log(int level, const char *file, int line, const char *function, const char *fmt, ...)
{
	struct ast_callid *callid;
	va_list ap;

	callid = ast_read_threadstorage_callid();

	va_start(ap, fmt);
	ast_log_full(level, file, line, function, callid, fmt, ap);
	va_end(ap);

	if (callid) {
		ast_callid_unref(callid);
	}
}

void ast_log_callid(int level, const char *file, int line, const char *function, struct ast_callid *callid, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	ast_log_full(level, file, line, function, callid, fmt, ap);
	va_end(ap);
}

#ifdef HAVE_BKTR

struct ast_bt *ast_bt_create(void)
{
	struct ast_bt *bt = ast_calloc(1, sizeof(*bt));
	if (!bt) {
		ast_log(LOG_ERROR, "Unable to allocate memory for backtrace structure!\n");
		return NULL;
	}

	bt->alloced = 1;

	ast_bt_get_addresses(bt);

	return bt;
}

int ast_bt_get_addresses(struct ast_bt *bt)
{
	bt->num_frames = backtrace(bt->addresses, AST_MAX_BT_FRAMES);

	return 0;
}

void *ast_bt_destroy(struct ast_bt *bt)
{
	if (bt->alloced) {
		ast_free(bt);
	}

	return NULL;
}

char **ast_bt_get_symbols(void **addresses, size_t num_frames)
{
	char **strings = NULL;
#if defined(BETTER_BACKTRACES)
	int stackfr;
	bfd *bfdobj;           /* bfd.h */
	Dl_info dli;           /* dlfcn.h */
	long allocsize;
	asymbol **syms = NULL; /* bfd.h */
	bfd_vma offset;        /* bfd.h */
	const char *lastslash;
	asection *section;
	const char *file, *func;
	unsigned int line;
	char address_str[128];
	char msg[1024];
	size_t strings_size;
	size_t *eachlen;
#endif

#if defined(BETTER_BACKTRACES)
	strings_size = num_frames * sizeof(*strings);
	eachlen = ast_calloc(num_frames, sizeof(*eachlen));

	if (!(strings = ast_calloc(num_frames, sizeof(*strings)))) {
		return NULL;
	}

	for (stackfr = 0; stackfr < num_frames; stackfr++) {
		int found = 0, symbolcount;

		msg[0] = '\0';

		if (!dladdr(addresses[stackfr], &dli)) {
			continue;
		}

		if (strcmp(dli.dli_fname, "asterisk") == 0) {
			char asteriskpath[256];
			if (!(dli.dli_fname = ast_utils_which("asterisk", asteriskpath, sizeof(asteriskpath)))) {
				/* This will fail to find symbols */
				ast_debug(1, "Failed to find asterisk binary for debug symbols.\n");
				dli.dli_fname = "asterisk";
			}
		}

		lastslash = strrchr(dli.dli_fname, '/');
		if (	(bfdobj = bfd_openr(dli.dli_fname, NULL)) &&
				bfd_check_format(bfdobj, bfd_object) &&
				(allocsize = bfd_get_symtab_upper_bound(bfdobj)) > 0 &&
				(syms = ast_malloc(allocsize)) &&
				(symbolcount = bfd_canonicalize_symtab(bfdobj, syms))) {

			if (bfdobj->flags & DYNAMIC) {
				offset = addresses[stackfr] - dli.dli_fbase;
			} else {
				offset = addresses[stackfr] - (void *) 0;
			}

			for (section = bfdobj->sections; section; section = section->next) {
				if (	!bfd_get_section_flags(bfdobj, section) & SEC_ALLOC ||
						section->vma > offset ||
						section->size + section->vma < offset) {
					continue;
				}

				if (!bfd_find_nearest_line(bfdobj, section, syms, offset - section->vma, &file, &func, &line)) {
					continue;
				}

				/* file can possibly be null even with a success result from bfd_find_nearest_line */
				file = file ? file : "";

				/* Stack trace output */
				found++;
				if ((lastslash = strrchr(file, '/'))) {
					const char *prevslash;
					for (prevslash = lastslash - 1; *prevslash != '/' && prevslash >= file; prevslash--);
					if (prevslash >= file) {
						lastslash = prevslash;
					}
				}
				if (dli.dli_saddr == NULL) {
					address_str[0] = '\0';
				} else {
					snprintf(address_str, sizeof(address_str), " (%p+%lX)",
						dli.dli_saddr,
						(unsigned long) (addresses[stackfr] - dli.dli_saddr));
				}
				snprintf(msg, sizeof(msg), "%s:%u %s()%s",
					lastslash ? lastslash + 1 : file, line,
					S_OR(func, "???"),
					address_str);

				break; /* out of section iteration */
			}
		}
		if (bfdobj) {
			bfd_close(bfdobj);
			if (syms) {
				ast_free(syms);
			}
		}

		/* Default output, if we cannot find the information within BFD */
		if (!found) {
			if (dli.dli_saddr == NULL) {
				address_str[0] = '\0';
			} else {
				snprintf(address_str, sizeof(address_str), " (%p+%lX)",
					dli.dli_saddr,
					(unsigned long) (addresses[stackfr] - dli.dli_saddr));
			}
			snprintf(msg, sizeof(msg), "%s %s()%s",
				lastslash ? lastslash + 1 : dli.dli_fname,
				S_OR(dli.dli_sname, "<unknown>"),
				address_str);
		}

		if (!ast_strlen_zero(msg)) {
			char **tmp;
			eachlen[stackfr] = strlen(msg);
			if (!(tmp = ast_realloc(strings, strings_size + eachlen[stackfr] + 1))) {
				ast_free(strings);
				strings = NULL;
				break; /* out of stack frame iteration */
			}
			strings = tmp;
			strings[stackfr] = (char *) strings + strings_size;
			ast_copy_string(strings[stackfr], msg, eachlen[stackfr] + 1);
			strings_size += eachlen[stackfr] + 1;
		}
	}

	if (strings) {
		/* Recalculate the offset pointers */
		strings[0] = (char *) strings + num_frames * sizeof(*strings);
		for (stackfr = 1; stackfr < num_frames; stackfr++) {
			strings[stackfr] = strings[stackfr - 1] + eachlen[stackfr - 1] + 1;
		}
	}
#else /* !defined(BETTER_BACKTRACES) */
	strings = backtrace_symbols(addresses, num_frames);
#endif /* defined(BETTER_BACKTRACES) */
	return strings;
}

#endif /* HAVE_BKTR */

void ast_backtrace(void)
{
#ifdef HAVE_BKTR
	struct ast_bt *bt;
	int i = 0;
	char **strings;

	if (!(bt = ast_bt_create())) {
		ast_log(LOG_WARNING, "Unable to allocate space for backtrace structure\n");
		return;
	}

	if ((strings = ast_bt_get_symbols(bt->addresses, bt->num_frames))) {
		ast_debug(1, "Got %d backtrace record%c\n", bt->num_frames, bt->num_frames != 1 ? 's' : ' ');
		for (i = 3; i < bt->num_frames - 2; i++) {
			ast_debug(1, "#%d: [%p] %s\n", i - 3, bt->addresses[i], strings[i]);
		}

		/* MALLOC_DEBUG will erroneously report an error here, unless we undef the macro. */
#undef free
		free(strings);
	} else {
		ast_debug(1, "Could not allocate memory for backtrace\n");
	}
	ast_bt_destroy(bt);
#else
	ast_log(LOG_WARNING, "Must run configure with '--with-execinfo' for stack backtraces.\n");
#endif /* defined(HAVE_BKTR) */
}

void __ast_verbose_ap(const char *file, int line, const char *func, int level, struct ast_callid *callid, const char *fmt, va_list ap)
{
	struct ast_str *buf = NULL;
	int res = 0;
	const char *prefix = level >= 4 ? VERBOSE_PREFIX_4 : level == 3 ? VERBOSE_PREFIX_3 : level == 2 ? VERBOSE_PREFIX_2 : level == 1 ? VERBOSE_PREFIX_1 : "";
	signed char magic = level > 127 ? -128 : -level - 1; /* 0 => -1, 1 => -2, etc.  Can't pass NUL, as it is EOS-delimiter */

	/* For compatibility with modules still calling ast_verbose() directly instead of using ast_verb() */
	if (level < 0) {
		if (!strncmp(fmt, VERBOSE_PREFIX_4, strlen(VERBOSE_PREFIX_4))) {
			magic = -5;
		} else if (!strncmp(fmt, VERBOSE_PREFIX_3, strlen(VERBOSE_PREFIX_3))) {
			magic = -4;
		} else if (!strncmp(fmt, VERBOSE_PREFIX_2, strlen(VERBOSE_PREFIX_2))) {
			magic = -3;
		} else if (!strncmp(fmt, VERBOSE_PREFIX_1, strlen(VERBOSE_PREFIX_1))) {
			magic = -2;
		} else {
			magic = -1;
		}
	}

	if (!(buf = ast_str_thread_get(&verbose_buf, VERBOSE_BUF_INIT_SIZE))) {
		return;
	}

	if (ast_opt_timestamp) {
		struct timeval now;
		struct ast_tm tm;
		char date[40];
		char *datefmt;

		now = ast_tvnow();
		ast_localtime(&now, &tm, NULL);
		ast_strftime(date, sizeof(date), dateformat, &tm);
		datefmt = ast_alloca(strlen(date) + 3 + strlen(prefix) + strlen(fmt) + 1);
		sprintf(datefmt, "%c[%s] %s%s", (char) magic, date, prefix, fmt);
		fmt = datefmt;
	} else {
		char *tmp = ast_alloca(strlen(prefix) + strlen(fmt) + 2);
		sprintf(tmp, "%c%s%s", (char) magic, prefix, fmt);
		fmt = tmp;
	}

	/* Build string */
	res = ast_str_set_va(&buf, 0, fmt, ap);

	/* If the build failed then we can drop this allocated message */
	if (res == AST_DYNSTR_BUILD_FAILED) {
		return;
	}

	ast_log_callid(__LOG_VERBOSE, file, line, func, callid, "%s", ast_str_buffer(buf));
}

void __ast_verbose(const char *file, int line, const char *func, int level, const char *fmt, ...)
{
	struct ast_callid *callid;
	va_list ap;

	callid = ast_read_threadstorage_callid();

	va_start(ap, fmt);
	__ast_verbose_ap(file, line, func, level, callid, fmt, ap);
	va_end(ap);

	if (callid) {
		ast_callid_unref(callid);
	}
}

void __ast_verbose_callid(const char *file, int line, const char *func, int level, struct ast_callid *callid, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	__ast_verbose_ap(file, line, func, level, callid, fmt, ap);
	va_end(ap);
}

/* No new code should use this directly, but we have the ABI for backwards compat */
#undef ast_verbose
void __attribute__((format(printf, 1,2))) ast_verbose(const char *fmt, ...);
void ast_verbose(const char *fmt, ...)
{
	struct ast_callid *callid;
	va_list ap;

	callid = ast_read_threadstorage_callid();

	va_start(ap, fmt);
	__ast_verbose_ap("", 0, "", 0, callid, fmt, ap);
	va_end(ap);

	if (callid) {
		ast_callid_unref(callid);
	}
}

int ast_register_verbose(void (*v)(const char *string))
{
	struct verb *verb;

	if (!(verb = ast_malloc(sizeof(*verb))))
		return -1;

	verb->verboser = v;

	AST_RWLIST_WRLOCK(&verbosers);
	AST_RWLIST_INSERT_HEAD(&verbosers, verb, list);
	AST_RWLIST_UNLOCK(&verbosers);

	return 0;
}

int ast_unregister_verbose(void (*v)(const char *string))
{
	struct verb *cur;

	AST_RWLIST_WRLOCK(&verbosers);
	AST_RWLIST_TRAVERSE_SAFE_BEGIN(&verbosers, cur, list) {
		if (cur->verboser == v) {
			AST_RWLIST_REMOVE_CURRENT(list);
			ast_free(cur);
			break;
		}
	}
	AST_RWLIST_TRAVERSE_SAFE_END;
	AST_RWLIST_UNLOCK(&verbosers);

	return cur ? 0 : -1;
}

static void update_logchannels(void)
{
	struct logchannel *cur;

	AST_RWLIST_WRLOCK(&logchannels);

	global_logmask = 0;

	AST_RWLIST_TRAVERSE(&logchannels, cur, list) {
		cur->logmask = make_components(cur->components, cur->lineno, &cur->verbosity);
		global_logmask |= cur->logmask;
	}

	AST_RWLIST_UNLOCK(&logchannels);
}

int ast_logger_register_level(const char *name)
{
	unsigned int level;
	unsigned int available = 0;

	AST_RWLIST_WRLOCK(&logchannels);

	for (level = 0; level < ARRAY_LEN(levels); level++) {
		if ((level >= 16) && !available && !levels[level]) {
			available = level;
			continue;
		}

		if (levels[level] && !strcasecmp(levels[level], name)) {
			ast_log(LOG_WARNING,
				"Unable to register dynamic logger level '%s': a standard logger level uses that name.\n",
				name);
			AST_RWLIST_UNLOCK(&logchannels);

			return -1;
		}
	}

	if (!available) {
		ast_log(LOG_WARNING,
			"Unable to register dynamic logger level '%s'; maximum number of levels registered.\n",
			name);
		AST_RWLIST_UNLOCK(&logchannels);

		return -1;
	}

	levels[available] = ast_strdup(name);

	AST_RWLIST_UNLOCK(&logchannels);

	ast_debug(1, "Registered dynamic logger level '%s' with index %d.\n", name, available);

	update_logchannels();

	return available;
}

void ast_logger_unregister_level(const char *name)
{
	unsigned int found = 0;
	unsigned int x;

	AST_RWLIST_WRLOCK(&logchannels);

	for (x = 16; x < ARRAY_LEN(levels); x++) {
		if (!levels[x]) {
			continue;
		}

		if (strcasecmp(levels[x], name)) {
			continue;
		}

		found = 1;
		break;
	}

	if (found) {
		/* take this level out of the global_logmask, to ensure that no new log messages
		 * will be queued for it
		 */

		global_logmask &= ~(1 << x);

		ast_free(levels[x]);
		levels[x] = NULL;
		AST_RWLIST_UNLOCK(&logchannels);

		ast_debug(1, "Unregistered dynamic logger level '%s' with index %d.\n", name, x);

		update_logchannels();
	} else {
		AST_RWLIST_UNLOCK(&logchannels);
	}
}

