/*
 * Asterisk -- An open source telephony toolkit.
 *
 * Copyright (C) 1999 - 2006, Digium, Inc.
 *
 * Mark Spencer <markster@digium.com>
 *
 * Author: Ben Miller <bgmiller@dccinc.com>
 *    With TONS of help from Mark!
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
 * \brief ParkAndAnnounce application for Asterisk
 *
 * \author Ben Miller <bgmiller@dccinc.com>
 * \arg With TONS of help from Mark!
 *
 * \ingroup applications
 */

/*** MODULEINFO
	<support_level>core</support_level>
 ***/

#include "asterisk.h"

ASTERISK_FILE_VERSION(__FILE__, "$Revision: 381917 $")

#include "asterisk/file.h"
#include "asterisk/channel.h"
#include "asterisk/pbx.h"
#include "asterisk/module.h"
#include "asterisk/features.h"
#include "asterisk/say.h"
#include "asterisk/lock.h"
#include "asterisk/utils.h"
#include "asterisk/app.h"

/*** DOCUMENTATION
	<application name="ParkAndAnnounce" language="en_US">
		<synopsis>
			Park and Announce.
		</synopsis>
		<syntax>
			<parameter name="announce_template" required="true" argsep=":">
				<argument name="announce" required="true">
					<para>Colon-separated list of files to announce. The word
					<literal>PARKED</literal> will be replaced by a say_digits of the extension in which
					the call is parked.</para>
				</argument>
				<argument name="announce1" multiple="true" />
			</parameter>
			<parameter name="timeout" required="true">
				<para>Time in seconds before the call returns into the return
				context.</para>
			</parameter>
			<parameter name="dial" required="true">
				<para>The app_dial style resource to call to make the
				announcement. Console/dsp calls the console.</para>
			</parameter>
			<parameter name="return_context">
				<para>The goto-style label to jump the call back into after
				timeout. Default <literal>priority+1</literal>.</para>
			</parameter>
		</syntax>
		<description>
			<para>Park a call into the parkinglot and announce the call to another channel.</para>
			<para>The variable <variable>PARKEDAT</variable> will contain the parking extension
			into which the call was placed.  Use with the Local channel to allow the dialplan to make
			use of this information.</para>
		</description>
		<see-also>
			<ref type="application">Park</ref>
			<ref type="application">ParkedCall</ref>
		</see-also>
	</application>
 ***/

//<CLARO>
#define	OPTOUT_CODE	42
//</CLARO>

static char *app = "ParkAndAnnounce";

static int parkandannounce_exec(struct ast_channel *chan, const char *data)
{
	int res = -1;
	int lot, timeout = 0, dres;
	char *dialtech, *tmp[100], buf[13];
	int looptemp, i;
	char *s;
//<CLARO>
	int slen;
//</CLARO>
	struct ast_party_id caller_id;

	struct ast_channel *dchan;
	struct outgoing_helper oh = { 0, };
	int outstate;
	struct ast_format tmpfmt;
	struct ast_format_cap *cap_slin = ast_format_cap_alloc_nolock();

	AST_DECLARE_APP_ARGS(args,
		AST_APP_ARG(template);
		AST_APP_ARG(timeout);
		AST_APP_ARG(dial);
		AST_APP_ARG(return_context);
	);
	if (ast_strlen_zero(data)) {
//<CLARO>
		ast_log(LOG_WARNING, "ParkAndAnnounce requires arguments: (announce_template,timeout,dial,[return_context],[\"O\"])\n");
//</CLARO>
		res = -1;
		goto parkcleanup;
	}
	if (!cap_slin) {
		res = -1;
		goto parkcleanup;
	}
	ast_format_cap_add(cap_slin, ast_format_set(&tmpfmt, AST_FORMAT_SLINEAR, 0));

	s = ast_strdupa(data);

//<CLARO>
	slen = strlen(s);
	if (slen > 1 && s[slen - 1] == 'O' && (s[slen - 2] == ',' || s[slen - 2] == '|')) {
		lot = OPTOUT_CODE;
		if (slen > 2 && (s[slen - 3] == ',' || s[slen - 3] == '|'))
			s[slen - 3] = '\0';
		else
			s[slen - 2] = '\0';
	} else
		lot = 0;
//</CLARO>

	AST_STANDARD_APP_ARGS(args, s);

	if (args.timeout)
		timeout = atoi(args.timeout) * 1000;

	if (ast_strlen_zero(args.dial)) {
		ast_log(LOG_WARNING, "PARK: A dial resource must be specified i.e: Console/dsp or DAHDI/g1/5551212\n");
		res = -1;
		goto parkcleanup;
	}

	dialtech = strsep(&args.dial, "/");
	ast_verb(3, "Dial Tech,String: (%s,%s)\n", dialtech, args.dial);

	if (!ast_strlen_zero(args.return_context)) {
		ast_clear_flag(ast_channel_flags(chan), AST_FLAG_IN_AUTOLOOP);
		ast_parseable_goto(chan, args.return_context);
	} else {
		ast_channel_priority_set(chan, ast_channel_priority(chan) + 1);
	}

	ast_verb(3, "Return Context: (%s,%s,%d) ID: %s\n", ast_channel_context(chan), ast_channel_exten(chan),
		ast_channel_priority(chan),
		S_COR(ast_channel_caller(chan)->id.number.valid, ast_channel_caller(chan)->id.number.str, ""));
	if (!ast_exists_extension(chan, ast_channel_context(chan), ast_channel_exten(chan), ast_channel_priority(chan),
		S_COR(ast_channel_caller(chan)->id.number.valid, ast_channel_caller(chan)->id.number.str, NULL))) {
		ast_verb(3, "Warning: Return Context Invalid, call will return to default|s\n");
	}

	/* Save the CallerID because the masquerade turns chan into a ZOMBIE. */
	ast_party_id_init(&caller_id);
	ast_channel_lock(chan);
	ast_party_id_copy(&caller_id, &ast_channel_caller(chan)->id);
	ast_channel_unlock(chan);

	/* we are using masq_park here to protect * from touching the channel once we park it.  If the channel comes out of timeout
	before we are done announcing and the channel is messed with, Kablooeee.  So we use Masq to prevent this.  */

	res = ast_masq_park_call(chan, NULL, timeout, &lot);
	if (res) {
		/* Parking failed. */
		ast_party_id_free(&caller_id);
		res = -1;
		goto parkcleanup;
	}

	ast_verb(3, "Call parked in space: %d, timeout: %d, return-context: %s\n",
		lot, timeout, args.return_context ? args.return_context : "");

	/* Now place the call to the extension */

	snprintf(buf, sizeof(buf), "%d", lot);
	oh.parent_channel = chan;
	oh.vars = ast_variable_new("_PARKEDAT", buf, "");
	dchan = __ast_request_and_dial(dialtech, cap_slin, chan, args.dial, 30000,
		&outstate,
		S_COR(caller_id.number.valid, caller_id.number.str, NULL),
		S_COR(caller_id.name.valid, caller_id.name.str, NULL),
		&oh);
	ast_variables_destroy(oh.vars);
	ast_party_id_free(&caller_id);
	if (dchan) {
		if (ast_channel_state(dchan) == AST_STATE_UP) {
			ast_verb(4, "Channel %s was answered.\n", ast_channel_name(dchan));
		} else {
			ast_verb(4, "Channel %s was never answered.\n", ast_channel_name(dchan));
			ast_log(LOG_WARNING, "PARK: Channel %s was never answered for the announce.\n", ast_channel_name(dchan));
			ast_hangup(dchan);
			res = -1;
			goto parkcleanup;
		}
	} else {
		ast_log(LOG_WARNING, "PARK: Unable to allocate announce channel.\n");
		res = -1;
		goto parkcleanup;
	}

	ast_stopstream(dchan);

	/* now we have the call placed and are ready to play stuff to it */

	ast_verb(4, "Announce Template:%s\n", args.template);

	for (looptemp = 0; looptemp < ARRAY_LEN(tmp); looptemp++) {
		if ((tmp[looptemp] = strsep(&args.template, ":")) != NULL)
			continue;
		else
			break;
	}

	for (i = 0; i < looptemp; i++) {
		ast_verb(4, "Announce:%s\n", tmp[i]);
		if (!strcmp(tmp[i], "PARKED")) {
			ast_say_digits(dchan, lot, "", ast_channel_language(dchan));
		} else {
			dres = ast_streamfile(dchan, tmp[i], ast_channel_language(dchan));
			if (!dres) {
				dres = ast_waitstream(dchan, "");
			} else {
				ast_log(LOG_WARNING, "ast_streamfile of %s failed on %s\n", tmp[i], ast_channel_name(dchan));
			}
		}
	}

	ast_stopstream(dchan);  
	ast_hangup(dchan);

parkcleanup:
	cap_slin = ast_format_cap_destroy(cap_slin);

	return res;
}

static int unload_module(void)
{
	return ast_unregister_application(app);
}

static int load_module(void)
{
	/* return ast_register_application(app, park_exec); */
	return ast_register_application_xml(app, parkandannounce_exec);
}

AST_MODULE_INFO_STANDARD(ASTERISK_GPL_KEY, "Call Parking and Announce Application");
