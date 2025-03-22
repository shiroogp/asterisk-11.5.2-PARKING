package biz.mmih.metmom.shared.services.vitel.asterisk.event.processor.core.impl;

import biz.mmih.metmom.shared.services.vitel.asterisk.event.processor.core.api.AsteriskEventClientPort;
import biz.mmih.metmom.shared.services.vitel.asterisk.event.processor.core.api.AsteriskInfraPort;
import biz.mmih.metmom.shared.services.vitel.asterisk.event.processor.core.api.Node;
import lombok.SneakyThrows;
import lombok.extern.slf4j.Slf4j;

import java.io.IOException;
import java.math.BigInteger;
import java.util.HashMap;
import java.util.Set;
import java.util.concurrent.ConcurrentHashMap;

import org.apache.tomcat.util.buf.StringUtils;
import org.asteriskjava.live.AsteriskChannel;
import org.asteriskjava.live.HangupCause;
import org.asteriskjava.manager.ManagerEventListener;
import org.asteriskjava.manager.action.DbPutAction;
import org.asteriskjava.manager.action.PJSipShowEndpointAction;
import org.asteriskjava.manager.action.SetVarAction;
import org.asteriskjava.manager.event.ConnectEvent;
import org.asteriskjava.manager.event.ContactStatusEvent;
import org.asteriskjava.manager.event.DeviceStateChangeEvent;
import org.asteriskjava.manager.event.DialBeginEvent;
import org.asteriskjava.manager.event.DialEndEvent;
import org.asteriskjava.manager.event.DialEvent;
import org.asteriskjava.manager.event.DialStateEvent;
import org.asteriskjava.manager.event.FullyBootedEvent;
import org.asteriskjava.manager.event.HangupEvent;
import org.asteriskjava.manager.event.HangupRequestEvent;
import org.asteriskjava.manager.event.LocalBridgeEvent;
import org.asteriskjava.manager.event.MonitorStartEvent;
import org.asteriskjava.manager.event.MonitorStopEvent;
import org.asteriskjava.manager.event.NewChannelEvent;
import org.asteriskjava.manager.event.NewExtenEvent;
import org.asteriskjava.manager.event.OriginateResponseEvent;
import org.asteriskjava.manager.event.PeerStatusEvent;
import org.asteriskjava.manager.event.SoftHangupRequestEvent;
import org.asteriskjava.manager.event.SuccessfulAuthEvent;
import org.asteriskjava.manager.event.VarSetEvent;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.core.env.Environment;
import org.springframework.http.ResponseEntity;
import org.springframework.stereotype.Service;
import org.springframework.web.client.RestClient;
import org.springframework.web.util.UriComponentsBuilder;

@Slf4j
@Service
class AsteriskEventClientAdaptor implements AsteriskEventClientPort {

    private final Environment env;

    private final AsteriskInfraPort asteriskInfraPort;
    ConcurrentHashMap<String, HashMap> channelvars = new ConcurrentHashMap<>();
    ConcurrentHashMap<String, AsteriskChannel> astChannels = new ConcurrentHashMap<>();
    ConcurrentHashMap<String, HashMap> linkedchannels = new ConcurrentHashMap<>();
    @Value("${api.host.baseurl}")
    private String baseURL;
    RestClient restClient;
    // = RestClient.builder()
    // .baseUrl(baseURL)
    // .build();
    // "http://vltelextdev202.metmom.mmih.biz:8086"

    static volatile String vitelog = new String("");

    AsteriskEventClientAdaptor(AsteriskInfraPort asteriskInfraPort, Environment env) {
        this.asteriskInfraPort = asteriskInfraPort;
        this.env = env;
        log.info("BaseURL: " + env.getProperty("api.host.baseurl"));
        restClient = RestClient.builder()
                .baseUrl(env.getProperty(
                        "api.host.baseurl"))
                .build();
    }

    @Override
    public void dialEvent(DialEvent newevent) {
        log.debug("Received a dialEvent: {}", newevent);
        // if (newevent.getDialString() != null) {
        if (newevent.getSubEvent().equalsIgnoreCase("Begin")) {
            // <VITEL>
            // HashMap<String, String> currentChannelVars =
            // getChannelVars(newevent.getUniqueId());
            // if (currentChannelVars == null) {
            // return;
            // }
            // // System.out.println("Current channel Vars: " + currentChannelVars);
            // // if (ast_channel_cdr(chan)) {
            // // struct ast_var_t *current;
            // // struct ast_var_t *newvar;
            // String varname;
            // int vartype;

            // // AST_LIST_TRAVERSE(ast_channel_varshead(chan), current, entries) {
            // // ast_vitel_log("VARS: ", ast_var_full_name(current),
            // ast_var_value(current),
            // // "TEST", "TEST", "TEST", "", "","");
            // // }
            // // ast_vitel_log(ast_channel_caller(chan)->id.number.str,
            // // ast_channel_dialed(chan)->number.str, exten, "TEST", "TEST", "TEST", "",
            // // "","");
            // String trExten = currentChannelVars.get("BLINDTRANSFER");
            // String txDst = currentChannelVars.get("MANUAL_BLIND_TRANSFER_DEST");
            // String txFail = currentChannelVars.get("TXFAIL");
            // String txNo = currentChannelVars.get("exten");
            // String temp1 = currentChannelVars.get("VITELVARS");
            // String temp2 = currentChannelVars.get("VITELVARS");//
            // pbx_builtin_getvar_helper(tmp->chan, "VITELVARS");
            // String cSrc = currentChannelVars.get("FROM");
            // String v4func = currentChannelVars.get("V4FUNC");
            // String vuserfield = currentChannelVars.get("vuserfield");
            // String caller = currentChannelVars.get("SRC");
            // String callee = currentChannelVars.get("EXTEN");
            // String altcaller = currentChannelVars.get("ALTCALLER");
            // String huntgroup = currentChannelVars.get("HUNTSTRATEGY");
            // String numsubst = currentChannelVars.get("DIALLED_EXTEN");
            // String huntgroupnumber = currentChannelVars.get("HUNTGROUPNUMBER");
            // if (numsubst == null) {
            // numsubst = "";
            // }
            // // Find the actual extension if non-local
            // String dialledNum = getPeerName(numsubst);
            // // if (dialledNum)
            // // {
            // // dialledNum++;
            // // }
            // // else
            // // {
            // // dialledNum = numsubst;
            // // }
            // // pbx_builtin_setvar_helper(tmp->chan, "__DIALLED_EXTEN", dialledNum);
            // String calleridnum = newevent.getCallerIdNum();
            // String exten = newevent.getDialString();// ast_channel_exten(chan)
            // String sourceChannel = newevent.getChannel();// ast_channel_name(chan)
            // String destChannel = newevent.getDestChannel();// ast_channel_name(tmp->chan)
            // String uniqueidChannel = newevent.getUniqueId();// ast_channel_uniqueid(chan)
            // // AsteriskChannel astChannel = getAstChannel(newevent.getUniqueId());
            // // astChannel.setVariable("__DIALLED_EXTENtest", dialledNum);
            // SetVarAction setvar = new SetVarAction(newevent.getDestChannel(),
            // "__DIALLED_EXTEN", dialledNum);
            // try {
            // asteriskInfraPort.sendAction(setvar);
            // } catch (Exception ex) {
            // log.error(ex.getStackTrace().toString());
            // }
            // //// pbx_builtin_setvar_helper(chan, "DIALLED_EXTEN", dialledNum);
            // setvar = new SetVarAction(newevent.getChannel(), "DIALLED_EXTENmctest",
            // dialledNum);
            // try {
            // asteriskInfraPort.sendAction(setvar);
            // } catch (Exception ex) {
            // log.error(ex.getStackTrace().toString());
            // }
            // if (caller != null) {
            // calleridnum = caller;
            // // sprintf(ast_channel_caller(chan)->id.number.str, "%s", caller);
            // }
            // if (callee != null) {
            // exten = callee;
            // // sprintf(ast_channel_exten(chan), "%s", callee);
            // }
            // String callingVars = "NONE";
            // String calledVars = "NONE";
            // if (temp1 != null && strlen(temp1) > 0) {
            // callingVars = temp1;
            // }
            // if (temp2 != null && strlen(temp2) > 0) {
            // calledVars = temp2;
            // }
            // if (v4func == null) {
            // v4func = "NONE";
            // }
            // // ast_channel_caller(chan)->id.number.str
            // if (strcmp(v4func, "TRANSFER")) {
            // // TODO
            // if (altcaller != null) {
            // // sprintf(ast_channel_caller(chan)->id.number.str, "%s", altcaller);
            // calleridnum = altcaller;
            // }
            // // sprintf(ast_channel_exten(chan), "%s", txNo);
            // exten = txNo;
            // if (strcmp(txFail, "true")) {
            // exten = trExten;
            // // sprintf(ast_channel_exten(chan), "%s", trExten);
            // }
            // if (huntgroup != null) {
            // ast_vitel_log(uniqueidChannel, "ORIGINATED", calleridnum, dialledNum,
            // "HUNTGROUP",
            // sourceChannel, destChannel, ((huntgroupnumber != null) ? huntgroupnumber :
            // exten), null,
            // null, null);
            // } else {
            // ast_vitel_log(uniqueidChannel, "ORIGINATED", calleridnum, exten, vuserfield,
            // callingVars,
            // calledVars, sourceChannel, destChannel, trExten, txFail);
            // }
            // } else {
            // if (cSrc != null && strcmp(cSrc, "QUEUE")) {
            // // System.out.println("VITEL LOG: " + newevent.getUniqueId() + "|" +
            // // "ORIGINATED" + "|" + calleridnum + "|" + exten + "|" +
            // newevent.getChannel()
            // // + "|" + newevent.getDestChannel() + "|" + callingVars + "|" + calledVars +
            // // "|" + "QUEUE");
            // ast_vitel_log(uniqueidChannel, "ORIGINATED", calleridnum, exten,
            // sourceChannel, destChannel,
            // callingVars, calledVars, "QUEUE", null, null);
            // } // TODO
            // else {
            // if (txDst != null) {
            // if (huntgroup != null) {
            // ast_vitel_log(uniqueidChannel, "ORIGINATED", calleridnum, dialledNum,
            // "HUNTGROUP",
            // sourceChannel, destChannel,
            // ((huntgroupnumber != null) ? huntgroupnumber : exten), null, null, null);
            // } else {
            // ast_vitel_log(uniqueidChannel, "ORIGINATED", calleridnum, txDst,
            // sourceChannel,
            // destChannel, callingVars, calledVars, null, null, null);
            // }
            // } else {
            // if (huntgroup != null) {
            // ast_vitel_log(uniqueidChannel, "ORIGINATED", calleridnum, dialledNum,
            // "HUNTGROUP",
            // sourceChannel, destChannel,
            // ((huntgroupnumber != null) ? huntgroupnumber : exten), null, null, null);
            // } else {
            // ast_vitel_log(uniqueidChannel, "ORIGINATED", calleridnum, exten,
            // sourceChannel,
            // destChannel, callingVars, calledVars, null, null, null);
            // }
            // }
            // }
            // }
            // }

            // </VITEL>
        } else if (newevent.getSubEvent().equalsIgnoreCase("End")) {
            // // <VITEL>
            // HashMap<String, String> currentChannelVars =
            // getChannelVars(newevent.getUniqueId());

            // String suppress = currentChannelVars.get("SUPPRESSVLOG");
            // if (suppress == null || !strcmp(suppress, "true")) {
            // // if (ast_channel_cdr(in)) {
            // String trExten = currentChannelVars.get("BLINDTRANSFER");
            // String txDst = currentChannelVars.get("MANUAL_BLIND_TRANSFER_DEST");
            // String txFail = currentChannelVars.get("TXFAIL");
            // String txNo = currentChannelVars.get("exten");
            // String temp1 = currentChannelVars.get("VITELVARS");
            // String temp2 = currentChannelVars.get("VITELVARS");
            // // TODO
            // // String temp2 = pbx_builtin_getvar_helper(peer, "VITELVARS");
            // String cSrc = currentChannelVars.get("FROM");
            // String v4func = currentChannelVars.get("V4FUNC");
            // String vuserfield = currentChannelVars.get("vuserfield");
            // String huntgroup = currentChannelVars.get("HUNTSTRATEGY");
            // String huntgroupnumber = currentChannelVars.get("HUNTGROUPNUMBER");
            // String caller = currentChannelVars.get("SRC");
            // String callee = currentChannelVars.get("EXTEN");
            // String tempNo = null; // TODO pbx_builtin_getvar_helper(c, "DIALLED_EXTEN");
            // String calleridnum = newevent.getCallerIdNum();
            // String exten = newevent.getConnectedLineNum();// ast_channel_exten(chan)
            // String sourceChannel = newevent.getChannel();// ast_channel_name(chan)
            // String destChannel = newevent.getDestChannel();// ast_channel_name(tmp->chan)
            // String uniqueidChannel = newevent.getUniqueId();// ast_channel_uniqueid(chan)

            // if (tempNo == null) {
            // tempNo = exten;
            // }
            // // Find the actual extension if non-local
            // String dialledNum = getPeerName(tempNo);

            // if (caller != null) {
            // calleridnum = caller;
            // // sprintf(ast_channel_caller(chan)->id.number.str, "%s", caller);
            // }
            // if (callee != null) {
            // exten = callee;
            // // sprintf(ast_channel_exten(chan), "%s", callee);
            // }
            // String callingVars = "NONE";
            // String calledVars = "NONE";
            // if (temp1 != null && strlen(temp1) > 0) {
            // callingVars = temp1;
            // }
            // if (temp2 != null && strlen(temp2) > 0) {
            // calledVars = temp2;
            // }
            // if (v4func == null) {
            // v4func = "NONE";
            // }

            // if (strcmp(v4func, "TRANSFER")) {
            // exten = txNo;
            // if (strcmp(txFail, "true")) {
            // exten = trExten;
            // }
            // if (huntgroup != null) {
            // ast_vitel_log(uniqueidChannel, "ANSWERED", calleridnum, dialledNum,
            // vuserfield,
            // "transferred", callingVars, calledVars,
            // sourceChannel,
            // destChannel, trExten, txFail, ((huntgroupnumber != null) ? huntgroupnumber
            // : exten));
            // } else {
            // ast_vitel_log(uniqueidChannel, "ANSWERED", calleridnum,
            // exten, vuserfield, "transferred", callingVars, calledVars,
            // sourceChannel,
            // destChannel, trExten, txFail);
            // }
            // } else {
            // if (cSrc != null && strcmp(cSrc, "QUEUE")) {
            // ast_vitel_log(uniqueidChannel, "ANSWERED", calleridnum,
            // exten,
            // sourceChannel,
            // destChannel, callingVars, calledVars,
            // "QUEUE");
            // } else {
            // if (txDst != null) {
            // if (huntgroup != null) {
            // ast_vitel_log(uniqueidChannel, "ANSWERED", calleridnum, dialledNum,
            // "HUNTGROUP", sourceChannel,
            // destChannel, ((huntgroupnumber != null) ? huntgroupnumber
            // : exten),
            // "");
            // } else {
            // ast_vitel_log(uniqueidChannel, "ANSWERED", calleridnum, txDst,
            // sourceChannel,
            // destChannel, callingVars, calledVars);
            // }
            // } else {
            // if (huntgroup != null) {
            // ast_vitel_log(uniqueidChannel, "ANSWERED", calleridnum, dialledNum,
            // "HUNTGROUP", sourceChannel,
            // destChannel, ((huntgroupnumber != null) ? huntgroupnumber
            // : exten));
            // } else {
            // ast_vitel_log(uniqueidChannel, "ANSWERED", calleridnum,
            // exten,
            // sourceChannel,
            // destChannel, callingVars, calledVars);
            // }
            // }
            // }
            // }
            // // }
            // }
            // // </VITEL>

        }
        // System.out.println(getChannelVars(newevent.getUniqueId()));

    }

    @Override
    public void newChannelEvent(NewChannelEvent event) {
        // log.debug("Received a newChannelEvent: {}", event);
        asteriskInfraPort.saveNewChannel(event.getChannel(), event.getUniqueId());
        updateChannelVars(event.getLinkedid(), "", "");
        updateChannelVars(event.getUniqueId(), "", "");
        updateLinkedChannels(event.getUniqueId(), event.getLinkedid());
    }

    @Override
    public void newExtenEvent(NewExtenEvent newExtenEvent) {
        log.debug("Received a newExtenEvent: {}", newExtenEvent);
    }

    @Override
    public void localBridgeEvent(LocalBridgeEvent localBridgeEvent) {
        log.debug("Received a localBridgeEvent: {}", localBridgeEvent);
        asteriskInfraPort.saveLocalBridge(localBridgeEvent.getChannel1(), localBridgeEvent.getChannel2());
    }

    @Override
    public void fullyBootedEvent(FullyBootedEvent fullyBootedEvent) {
        // log.debug("Received a fullyBootedEvent: {}", fullyBootedEvent);
    }

    @Override
    public void successfulAuthEvent(SuccessfulAuthEvent newevent) {
        // log.info("Received a successfulAuthEvent: {}", newevent);
        // PJSipShowEndpointAction setvar = new PJSipShowEndpointAction();
        // setvar.setEndpoint(newevent.getAccountId());
        // try {
        // asteriskInfraPort.sendAction(setvar);
        // } catch (Exception ex) {
        // log.error(ex.getStackTrace().toString());
        // }

        // if (newevent.getPeerStatus().equalsIgnoreCase("Unreachable")) {
        // // System.out.println("VITEL LOG: " + "N/A" + "|" + "DEREGISTER" + "|" +
        // // getPeerName(newevent.getPeer()) + "|"
        // // + "" + "|" + "" + "|" + "" + "|" + "" + "|" + "");
        // ast_vitel_log(
        // "N/A",
        // "DEREGISTER", getPeerName(newevent
        // .getPeer()),
        // "", "",
        // "", "", "");
        // } else if (newevent.getPeerStatus().equalsIgnoreCase("Reachable")) {
        // // System.out.println("VITEL LOG: " + "N/A" + "|" + "REGISTER" + "|" +
        // // getPeerName(newevent.getPeer()) + "|"
        // // + "" + "|" + "" + "|" + "" + "|" + "" + "|" + "");
        // ast_vitel_log(
        // "N/A",
        // "REGISTER", getPeerName(newevent
        // .getPeer()),
        // "", "",
        // "", "", "");
        // }
    }

    @Override
    public void contactStatusEvent(ContactStatusEvent newevent) {
        // log.info("Received a ContactStatusEvent: {}", newevent);
        // if (newevent.getPeerStatus().equalsIgnoreCase("Unreachable")) {
        // // System.out.println("VITEL LOG: " + "N/A" + "|" + "DEREGISTER" + "|" +
        // // getPeerName(newevent.getPeer()) + "|"
        // // + "" + "|" + "" + "|" + "" + "|" + "" + "|" + "");
        // ast_vitel_log(
        // "N/A",
        // "DEREGISTER", getPeerName(newevent
        // .getPeer()),
        // "", "",
        // "", "", "");
        // } else if (newevent.getPeerStatus().equalsIgnoreCase("Reachable")) {
        // // System.out.println("VITEL LOG: " + "N/A" + "|" + "REGISTER" + "|" +
        // // getPeerName(newevent.getPeer()) + "|"
        // // + "" + "|" + "" + "|" + "" + "|" + "" + "|" + "");
        // ast_vitel_log(
        // "N/A",
        // "REGISTER", getPeerName(newevent
        // .getPeer()),
        // "", "",
        // "", "", "");
        // }
    }

    @Override
    public void connectEvent(ConnectEvent connectEvent) {
        // log.debug("Received a connectEvent: {}", connectEvent);
    }

    @SneakyThrows
    @Override
    public void softHangupRequestEvent(SoftHangupRequestEvent softHangupRequestEvent) {
        // log.debug("Received a softHangupRequestEvent: {}", softHangupRequestEvent);
    }

    @Override
    public void hangupEvent(HangupEvent newevent) {
        try {
            String channelId = newevent.getChannel();
            String bridgedTo = getLinkedChannelsMap(channelId) != null ? getLinkedChannelsMap(channelId).get(channelId)
                    : null;
            if (bridgedTo != null && !bridgedTo.isEmpty()) {
                log.info("{} issued BYE while talking to {}", channelId, bridgedTo);
            } else {
                log.info("{} issued BYE", channelId);
            }

            // Set HANGUP_ORIGINATOR variable

            setVariable(channelId, "HANGUP_ORIGINATOR", "TRUE");
        } catch (Exception e) {
            logger.error("Error handling hangup event", e);
        }
        // log.debug("Received a hangupEvent: {} {}", newevent.getChannel(), newevent);
        // String suppress = "";
        // String PING = "";
        // String register_exten = "";
        // String hangup = "";
        // String tempNo = "";
        // // struct ast_channel *bridged;
        // // const char* numsubst = currentChannelVars.get("DIALEDPEERNUMBER");
        // String numsubst = "";
        // String calleridnum = "";
        // String exten = "";
        // String sourceChannel = "";
        // try {
        // // <VITEL>
        // HashMap<String, String> currentChannelVars =
        // getChannelVars(newevent.getUniqueId());
        // if (currentChannelVars != null) {

        // suppress = currentChannelVars.get("SUPPRESSVLOG");
        // PING = currentChannelVars.get("PING");
        // register_exten = currentChannelVars.get("REGISTER_EXTEN");
        // hangup = currentChannelVars.get("HANGUP_ORIGINATOR");
        // tempNo = currentChannelVars.get("DIALEDPEERNUMBER");
        // // struct ast_channel *bridged;
        // // const char* numsubst = currentChannelVars.get("DIALEDPEERNUMBER");
        // numsubst = "";
        // calleridnum = newevent.getCallerIdNum();
        // exten = newevent.getExten();// ast_channel_exten(chan)
        // sourceChannel = newevent.getChannel();// ast_channel_name(chan)
        // // String destChannel =
        // newevent.getDestChannel();//ast_channel_name(tmp->chan)
        // }

        // try {
        // for (String linkedid : getLinkedChannelsSet(newevent.getUniqueId())) {
        // HashMap<String, String> channelVars2 = getChannelVars(linkedid);
        // if (channelVars2 != null) {
        // if (channelVars2.get("SUPPRESSVLOG") != null &&
        // channelVars2.get("SUPPRESSVLOG").length() > 0) {
        // suppress = channelVars2.get("SUPPRESSVLOG");
        // }
        // if (channelVars2.get("PING") != null && channelVars2.get("PING").length() >
        // 0) {
        // PING = channelVars2.get("PING");
        // }
        // if (channelVars2.get("REGISTER_EXTEN") != null
        // && channelVars2.get("REGISTER_EXTEN").length() > 0) {
        // register_exten = channelVars2.get("REGISTER_EXTEN");
        // }
        // if (channelVars2.get("HANGUP_ORIGINATOR") != null
        // && channelVars2.get("HANGUP_ORIGINATOR").length() > 0) {
        // hangup = channelVars2.get("HANGUP_ORIGINATOR");
        // }
        // if (channelVars2.get("DIALEDPEERNUMBER") != null
        // && channelVars2.get("DIALEDPEERNUMBER").length() > 0) {
        // tempNo = channelVars2.get("DIALEDPEERNUMBER");
        // }
        // }
        // }
        // } catch (Exception e) {
        // // TODO Auto-generated catch block
        // log.debug("No linked channel vars");
        // }

        // // AST_LIST_TRAVERSE(ast_channel_varshead(chan), current, entries)
        // // {
        // // // ast_vitel_log("VARS: ", ast_var_full_name(current),
        // // ast_var_value(current), "TEST", "TEST", "TEST", "", "","");
        // // if (strcmp(ast_var_full_name(current), "SUPPRESSVLOG") == 0)
        // // {
        // // suppress = ast_var_value(current);
        // // }
        // // if (strcmp(ast_var_full_name(current), "PING") == 0)
        // // {
        // // PING = ast_var_value(current);
        // // }
        // // if (strcmp(ast_var_full_name(current), "REGISTER_EXTEN") == 0)
        // // {
        // // register_exten = ast_var_value(current);
        // // }
        // // if (strcmp(ast_var_full_name(current), "HANGUP_ORIGINATOR") == 0)
        // // {
        // // hangup = ast_var_value(current);
        // // }
        // // if (strcmp(ast_var_full_name(current), "DIALEDPEERNUMBER") == 0)
        // // {
        // // tempNo = ast_var_value(current);
        // // }
        // // }
        // if (PING == null || PING.length() == 0) {
        // if (tempNo == null || tempNo.length() == 0) {
        // if (currentChannelVars.get("exten") != null &&
        // currentChannelVars.get("exten").length() > 0) {
        // tempNo = currentChannelVars.get("exten");
        // numsubst = getPeerName(currentChannelVars.get("exten"));
        // }
        // // if (ast_channel_exten(chan))
        // // {
        // // tempNo = ast_channel_exten(chan);
        // // numsubst = strchr(tempNo, '/');
        // // }
        // // if (ast_channel_dialed(chan)->number.str){
        // // tempNo = ast_channel_dialed(chan)->number.str;
        // // numsubst = strchr(tempNo, '/');
        // // }
        // // if (ast_channel_cdr(chan))
        // // {
        // // tempNo = ast_channel_cdr(chan)->dst;
        // // numsubst = strchr(tempNo, '/');
        // // }
        // }

        // // Find the actual extension if non-local
        // // if (tempNo == null || tempNo.length() == 0)
        // // {
        // // // numsubst = strchr(tempNo, '/');
        // // numsubst++;
        // // // if (numsubst)
        // // // {
        // // // numsubst++;
        // // // }
        // // // else
        // // // {
        // // // numsubst = tempNo;
        // // // }
        // // }
        // }
        // if (hangup == null || hangup.length() == 0) {
        // hangup = "FALSE";
        // }

        // if (suppress == null || suppress.length() == 0 || strcmp(suppress, "true")) {
        // // ast_vitel_log(ast_channel_caller(chan)->id.number.str,
        // // ast_channel_dialed(chan)->number.str, ast_channel_exten(chan), PING,
        // // ast_cause2str(ast_channel_hangupcause(chan)), PING, "", "%s","");

        // String status = currentChannelVars.get("DSTATUS");
        // String retryDial = currentChannelVars.get("RETRYDIAL");
        // // HangupCause hangupCause = HangupCause.getByCode(newevent.getCause());
        // System.out.println(newevent.getCauseTxt());

        // if (status == null || status.length() == 0) {
        // if (PING != null && PING.length() > 0) {
        // ast_vitel_log("NONE", "HANGUP", "0000", PING, newevent.getCauseTxt(), PING,
        // "", "%s", "");
        // } else if (retryDial != null && retryDial.length() > 0) {
        // if (numsubst != null && numsubst.length() > 0) {
        // ast_vitel_log("NONE", "HANGUP", calleridnum, numsubst,
        // newevent.getCauseTxt(),
        // sourceChannel, hangup, "%s", "");
        // } else {
        // ast_vitel_log("NONE", "HANGUP", calleridnum, retryDial,
        // newevent.getCauseTxt(),
        // sourceChannel, hangup, "%s", "");
        // }
        // } else if (PING != null && PING.length() > 0) {
        // ast_vitel_log("NONE", "HANGUP", "0000", PING, newevent.getCauseTxt(), PING,
        // "", "%s", "");
        // } else {
        // if (numsubst != null && numsubst.length() > 0) {
        // // ast_vitel_log("NONE", "HANGUP", ast_channel_dialed(chan)->number.str,
        // hangup,
        // // newevent.getCauseTxt(), sourceChannel, hangup, "", "");
        // // ast_vitel_log("NONE", "HANGUP", ast_channel_dialed(chan), hangup,
        // // newevent.getCauseTxt(), sourceChannel, hangup, "", "");
        // } else {
        // ast_vitel_log("NONE", "HANGUP", exten, hangup, newevent.getCauseTxt(),
        // sourceChannel,
        // hangup, "", "");
        // ast_vitel_log("NONE", "HANGUP", exten, hangup, newevent.getCauseTxt(),
        // sourceChannel,
        // hangup, "", "");
        // }
        // }
        // } else {
        // if (!strcmp(status, "FAIL")) {
        // ast_vitel_log("NONE", "HANGUP", calleridnum, "FAIL", newevent.getCauseTxt(),
        // sourceChannel,
        // hangup, "", "");
        // }
        // }
        // if (register_exten != null) {
        // ast_vitel_log("N/A", "REGISTER", register_exten, "", "", "", "", "", "");
        // }
        // if (PING != null && PING.length() > 0) {
        // removeChannelVars(newevent.getUniqueId());
        // removeLinkedChannelsMap(newevent.getUniqueId());

        // }
        // }
        // // </VITEL>
        // } catch (Exception e) {
        // // TODO: handle exception
        // e.printStackTrace();
        // }

        asteriskInfraPort.logStorageMaps();
    }

    private void setVariable(String channelId, String varName, String varValue) {
        // TODO Auto-generated method stub
        SetVarAction setvar = new SetVarAction(channelId, varName, varValue);

        try {
            asteriskInfraPort.sendAction(setvar);
        } catch (Exception ex) {
            log.error(ex.getStackTrace().toString());
        }
    }

    @Override
    public void originateResponseEvent(OriginateResponseEvent originateResponseEvent) {
        // log.debug("Received a originateResponseEvent: {}", originateResponseEvent);
    }

    @Override
    public void deviceStateChangeEvent(DeviceStateChangeEvent deviceStateChangeEvent) {
        // log.debug("Received a deviceStateChangeEvent: {}", deviceStateChangeEvent);
    }

    @Override
    public void varSetEvent(VarSetEvent event) {
        // log.debug("Received a varSetEvent: {}", event);
        asteriskInfraPort.saveVarSet(event.getChannel(), event.getVariable(), event.getValue());
        updateChannelVars(event.getLinkedId(), event.getVariable(), event.getValue());
        updateChannelVars(event.getUniqueId(), event.getVariable(), event.getValue());
        updateLinkedChannels(event.getUniqueId(), event.getLinkedId());

    }

    @Override
    public void setEventListener(ManagerEventListener eventListener) {
        asteriskInfraPort.setAsteriskServerEventListener(eventListener);
    }

    @Override
    public void dialStateEvent(DialStateEvent newevent) {
        // TODO Auto-generated method stub
        if (newevent.getDialStatus().equalsIgnoreCase("Ringing")) {
            HashMap<String, String> currentChannelVars = getChannelVars(newevent.getLinkedId());
            // System.out.println("Current channel Vars: " + currentChannelVars);

            String huntgroup = currentChannelVars.get("HUNTSTRATEGY");
            String suppress = currentChannelVars.get("SUPPRESSVLOG");
            if (suppress == null || !strcmp(suppress, "true")) {
                if (true) { // if ((loopIter == 0 || (huntgroup && !strcmp(huntgroup, "RINGALL"))))

                    String trExten = currentChannelVars.get("BLINDTRANSFER");
                    String txDst = currentChannelVars.get("MANUAL_BLIND_TRANSFER_DEST");
                    String txFail = currentChannelVars.get("TXFAIL");
                    String txNo = currentChannelVars.get("exten");
                    String temp1 = currentChannelVars.get("VITELVARS");
                    String temp2 = currentChannelVars.get("VITELVARS");
                    // TODO get correct value for temp2
                    // String temp2 = pbx_builtin_getvar_helper(c, "VITELVARS");
                    String cSrc = currentChannelVars.get("FROM");
                    String v4func = currentChannelVars.get("V4FUNC");
                    String vuserfield = currentChannelVars.get("vuserfield");
                    String caller = currentChannelVars.get("SRC");
                    String callee = currentChannelVars.get("EXTEN");
                    String numsubst = currentChannelVars.get("DIALLED_EXTEN");
                    // TODO get correct value
                    // String numsubst = pbx_builtin_getvar_helper(c, "DIALLED_EXTEN");
                    String huntgroupnumber = currentChannelVars.get("HUNTGROUPNUMBER");

                    String calleridnum = newevent.getCallerIdNum();// ast_channel_caller(in)->id.number.str
                    String exten = newevent.getExten();// ast_channel_exten(chan)
                    String sourceChannel = newevent.getChannel();// ast_channel_name(chan)
                    String destChannel = newevent.getDestChannel();// ast_channel_name(tmp->chan)
                    String uniqueidChannel = newevent.getUniqueId();// ast_channel_uniqueid(chan)
                    if (numsubst == null) {
                        numsubst = "";
                    }

                    // Find the actual extension if non-local
                    String dialledNum = getPeerName(numsubst);

                    // String dialledNum = strchr(numsubst, '/');
                    // if (dialledNum)
                    // {
                    // dialledNum++;
                    // }
                    // else
                    // {
                    // dialledNum = numsubst;
                    // }
                    if (caller != null) {
                        calleridnum = caller;
                        // sprintf(ast_channel_caller(chan)->id.number.str, "%s", caller);
                    }
                    if (callee != null) {
                        exten = callee;
                        // sprintf(ast_channel_exten(chan), "%s", callee);
                    }

                    // if (caller != null)
                    // {
                    // sprintf(ast_channel_caller(in)->id.number.str, "%s", caller);
                    // }
                    // if (callee != null)
                    // {
                    // sprintf(ast_channel_exten(in), "%s", callee);
                    // }
                    // pbx_builtin_setvar_helper(in, "SUCCESS", "true");
                    // AsteriskChannel astChannel = getAstChannel(newevent.getUniqueId());
                    // astChannel.setVariable("__DIALLED_EXTENtest", dialledNum);
                    SetVarAction setvar = new SetVarAction(newevent.getChannel(), "SUCCESS", "true");

                    try {
                        asteriskInfraPort.sendAction(setvar);
                    } catch (Exception ex) {
                        log.error(ex.getStackTrace().toString());
                    }

                    String callingVars = "NONE";
                    String calledVars = "NONE";
                    if (temp1 != null && strlen(temp1) > 0) {
                        callingVars = temp1;
                    }
                    if (temp2 != null && strlen(temp2) > 0) {
                        calledVars = temp2;
                    }
                    if (v4func == null) {
                        v4func = "NONE";
                    }
                    // if (temp1 && strlen(temp1) > 0)
                    // {
                    // callingVars = temp1;
                    // }
                    // if (temp2 && strlen(temp2) > 0)
                    // {
                    // calledVars = temp2;
                    // }
                    // if (!v4func)
                    // {
                    // v4func = "NONE";
                    // }

                    if (strcmp(v4func, "TRANSFER")) {
                        if (strcmp(calleridnum, calleridnum)) {
                            if (strlen(calleridnum) > 0) {
                                calleridnum = calleridnum;
                                // sprintf(ast_channel_caller(in)->id.number.str, "%s",
                                // ast_channel_caller(in)->id.number.str);
                            }
                        }
                        // Agent no longer has transfer pending, so update this
                        ast_db_put("transferPending", trExten, "no");
                        exten = txNo;
                        // sprintf(ast_channel_exten(in), "%s", txNo);
                        if (strcmp(txFail, "true")) {
                            exten = trExten;
                            // sprintf(ast_channel_exten(in), "%s", trExten);
                            // pbx_builtin_setvar_helper(in, "SUCCESS", "false");
                            setvar = new SetVarAction(newevent.getChannel(), "SUCCESS", "false");

                            try {
                                asteriskInfraPort.sendAction(setvar);
                            } catch (Exception ex) {
                                log.error(ex.getStackTrace().toString());
                            }

                        }
                        if (huntgroup != null) {
                            ast_vitel_log(uniqueidChannel, "RINGING", calleridnum, dialledNum, "HUNTGROUP", "ringing",
                                    callingVars, calledVars, sourceChannel, destChannel, trExten, txFail,
                                    ((huntgroupnumber != null) ? huntgroupnumber : exten));
                            ast_vitel_log(uniqueidChannel, "RINGING", calleridnum, dialledNum, "HUNTGROUP",
                                    "transferred", callingVars, calledVars, sourceChannel, destChannel, trExten,
                                    txFail);
                        } else {
                            ast_vitel_log(uniqueidChannel, "RINGING", calleridnum, exten, vuserfield, "ringing",
                                    callingVars, calledVars, sourceChannel, destChannel, trExten, txFail);
                            ast_vitel_log(uniqueidChannel, "RINGING", calleridnum, exten, vuserfield, "transferred",
                                    callingVars, calledVars, sourceChannel, destChannel, trExten, txFail);
                        }
                    } else {
                        if (cSrc != null && strcmp(cSrc, "QUEUE")) {
                            ast_vitel_log(uniqueidChannel, "RINGING", calleridnum, exten, sourceChannel, destChannel,
                                    callingVars, calledVars, "QUEUE");
                        } else {
                            if (txDst != null) {
                                if (huntgroup != null) {
                                    ast_vitel_log(uniqueidChannel, "RINGING", calleridnum, txDst, "HUNTGROUP",
                                            sourceChannel, destChannel,
                                            ((huntgroupnumber != null) ? huntgroupnumber : exten));
                                } else {
                                    ast_vitel_log(uniqueidChannel, "RINGING", calleridnum, txDst, sourceChannel,
                                            destChannel, callingVars, calledVars);
                                }
                            } else {
                                if (huntgroup != null) {
                                    ast_vitel_log(uniqueidChannel, "RINGING", calleridnum, dialledNum, "HUNTGROUP",
                                            sourceChannel, destChannel,
                                            ((huntgroupnumber != null) ? huntgroupnumber : exten));
                                } else {
                                    ast_vitel_log(uniqueidChannel, "RINGING", calleridnum, exten, sourceChannel,
                                            destChannel, callingVars, calledVars);
                                }
                            }
                        }
                    }
                }
            }

        }
    }

    @Override
    public void peerStatusEvent(PeerStatusEvent newevent) {
        System.out.println("Peer status event      -            " + newevent);
        // TODO Auto-generated method stub
        if (newevent.getPeerStatus().equalsIgnoreCase("Unreachable")) {
            // System.out.println("VITEL LOG: " + "N/A" + "|" + "DEREGISTER" + "|" +
            // getPeerName(newevent.getPeer()) + "|"
            // + "" + "|" + "" + "|" + "" + "|" + "" + "|" + "");
            ast_vitel_log(
                    "N/A",
                    "DEREGISTER", getPeerName(newevent
                            .getPeer()),
                    "", "",
                    "", "", "");
        } else if (newevent.getPeerStatus().equalsIgnoreCase("Reachable")) {
            // System.out.println("VITEL LOG: " + "N/A" + "|" + "REGISTER" + "|" +
            // getPeerName(newevent.getPeer()) + "|"
            // + "" + "|" + "" + "|" + "" + "|" + "" + "|" + "");
            ast_vitel_log(
                    "N/A",
                    "REGISTER", getPeerName(newevent
                            .getPeer()),
                    "", "",
                    "", "", "");
        }
    }

    public String getPeerName(String fullname) {
        return fullname.substring(fullname.indexOf("/") + 1);
    }

    public void updateChannelVars(String uniqueid, String varname, String varvalue) {
        System.out.println("Setting values: " + uniqueid + " - " + varname + " - " +
                varvalue);
        synchronized (channelvars) {
            if (varname.length() > 0) {
                HashMap<String, String> channelvarmap = channelvars.get(uniqueid);
                if (channelvarmap == null) {
                    channelvarmap = new HashMap<>();
                }
                channelvarmap.put(varname, varvalue);
                if (varname.startsWith("__")) {
                    channelvarmap.put(varname.substring(2), varvalue);
                }
                channelvars.put(uniqueid, channelvarmap);

            } else {
                HashMap<String, String> newMap = channelvars.get(uniqueid);
                if (newMap == null) {
                    newMap = new HashMap<>();
                    channelvars.put(uniqueid, newMap);

                }
            }
            // System.out.println("Channel vars: " + channelvars.get(uniqueid));
        }
    }

    public HashMap<String, String> getChannelVars(String uniqueid) {
        try {
            synchronized (channelvars) {
                return channelvars.get(uniqueid);
            }
        } catch (Exception e) {
            // TODO Auto-generated catch block
            // e.printStackTrace();
            return null;
        }
    }

    public HashMap<String, String> removeChannelVars(String uniqueid) {
        try {
            synchronized (channelvars) {
                return channelvars.remove(uniqueid);
            }
        } catch (Exception e) {
            // TODO Auto-generated catch block
            // e.printStackTrace();
            return null;
        }
    }

    public void updateLinkedChannels(String uniqueid, String linkeduniqueid) {
        // System.out.println("Setting values: " + uniqueid + " - " + varname + " - " +
        // varvalue);
        synchronized (linkedchannels) {
            HashMap<String, String> linkedmap = linkedchannels.get(linkeduniqueid);
            if (linkedmap == null) {
                linkedmap = new HashMap<>();
            }
            linkedmap.put(uniqueid, linkeduniqueid);
            linkedchannels.put(linkeduniqueid, linkedmap);

        }
        // System.out.println("Channel vars: " + channelvars.get(uniqueid));

    }

    public HashMap<String, String> getLinkedChannelsMap(String uniqueid) {
        try {
            synchronized (linkedchannels) {
                return linkedchannels.get(uniqueid);
            }
        } catch (Exception e) {
            // TODO Auto-generated catch block
            // e.printStackTrace();
            return null;
        }
    }

    public HashMap<String, String> removeLinkedChannelsMap(String uniqueid) {
        try {
            synchronized (linkedchannels) {
                return linkedchannels.remove(uniqueid);
            }
        } catch (Exception e) {
            // TODO Auto-generated catch block
            // e.printStackTrace();
            return null;
        }
    }

    public Set<String> getLinkedChannelsSet(String uniqueid) {
        try {
            synchronized (linkedchannels) {
                return linkedchannels.get(uniqueid).keySet();
            }
        } catch (Exception e) {
            // TODO Auto-generated catch block
            // e.printStackTrace();
            return null;
        }
    }

    public void updateAsteriskChannels(String uniqueid, AsteriskChannel newChannel) {
        synchronized (astChannels) {
            AsteriskChannel newMap = astChannels.get(uniqueid);
            if (newMap == null) {
                astChannels.put(uniqueid, newChannel);

            }
            // System.out.println("Asterisk Channel : " + astChannels.get(uniqueid));
        }
    }

    public AsteriskChannel getAstChannel(String uniqueid) {
        synchronized (astChannels) {
            return astChannels.get(uniqueid);
        }
    }

    private int strlen(String temp1) {
        return temp1.length();
    }

    private boolean strcmp(String srcString, String compString) {
        return srcString.equalsIgnoreCase(compString);
    }

    public void ast_vitel_log(String s1, String s2, String s3, String s4, String s5, String s6, String s7, String s8,
            String s9, String s10, String s11, String s12, String s13) {
        ast_vitel_log(s1, s2, s3, s4, s5, s6, s7, s8, s9, s10, s11, s12, s13, null, null, null, null, null, null, null);
    }

    public void ast_vitel_log(String s1, String s2, String s3, String s4, String s5, String s6, String s7, String s8,
            String s9, String s10, String s11, String s12) {
        ast_vitel_log(s1, s2, s3, s4, s5, s6, s7, s8, s9, s10, s11, s12, null, null, null, null, null, null, null,
                null);
    }

    public void ast_vitel_log(String s1, String s2, String s3, String s4, String s5, String s6, String s7, String s8,
            String s9, String s10, String s11) {
        ast_vitel_log(s1, s2, s3, s4, s5, s6, s7, s8, s9, s10, s11, null, null, null, null, null, null, null, null,
                null);
    }

    public void ast_vitel_log(String s1, String s2, String s3, String s4, String s5, String s6, String s7, String s8,
            String s9, String s10) {
        ast_vitel_log(s1, s2, s3, s4, s5, s6, s7, s8, s9, s10, null, null, null, null, null, null, null, null, null,
                null);
    }

    public void ast_vitel_log(String s1, String s2, String s3, String s4, String s5, String s6, String s7, String s8,
            String s9) {
        ast_vitel_log(s1, s2, s3, s4, s5, s6, s7, s8, s9, null, null, null, null, null, null, null, null, null, null,
                null);
    }

    public void ast_vitel_log(String s1, String s2, String s3, String s4, String s5, String s6, String s7, String s8) {
        ast_vitel_log(s1, s2, s3, s4, s5, s6, s7, s8, null, null, null, null, null, null, null, null, null, null, null,
                null);
    }

    public void ast_vitel_log(String s1, String s2, String s3, String s4, String s5, String s6, String s7) {
        ast_vitel_log(s1, s2, s3, s4, s5, s6, s7, null, null, null, null, null, null, null, null, null, null, null,
                null, null);
    }

    public void ast_vitel_log(String s1, String s2, String s3, String s4, String s5, String s6, String s7, String s8,
            String s9, String s10, String s11, String s12, String s13, String s14, String s15, String s16, String s17,
            String s18, String s19, String s20) {
        vitelog = "" + System.currentTimeMillis();
        if (s1 != null) {
            vitelog += "|" + s1;
        }
        if (s2 != null) {
            vitelog += "|" + s2;
        }
        if (s3 != null) {
            vitelog += "|" + s3;
        }
        if (s4 != null) {
            vitelog += "|" + s4;
        }
        if (s5 != null) {
            vitelog += "|" + s5;
        }
        if (s6 != null) {
            vitelog += "|" + s6;
        }
        if (s7 != null) {
            vitelog += "|" + s7;
        }
        if (s8 != null) {
            vitelog += "|" + s8;
        }
        if (s9 != null) {
            vitelog += "|" + s9;
        }
        if (s10 != null) {
            vitelog += "|" + s10;
        }
        if (s11 != null) {
            vitelog += "|" + s11;
        }
        if (s12 != null) {
            vitelog += "|" + s12;
        }
        if (s13 != null) {
            vitelog += "|" + s13;
        }
        if (s14 != null) {
            vitelog += "|" + s14;
        }
        if (s15 != null) {
            vitelog += "|" + s15;
        }
        if (s16 != null) {
            vitelog += "|" + s16;
        }
        if (s17 != null) {
            vitelog += "|" + s17;
        }
        if (s18 != null) {
            vitelog += "|" + s18;
        }
        if (s19 != null) {
            vitelog += "|" + s19;
        }
        if (s20 != null) {
            vitelog += "|" + s20;
        }
        log.info("VITEL LOG - " + vitelog);

        String pathVariable = "asterisk/events";
        ResponseEntity<Void> response = restClient.post()
                .uri(uriBuilder -> uriBuilder
                        .path("/" + pathVariable)
                        .queryParam("asteriskevent", vitelog)
                        // .queryParam("param2", "value2")
                        // .queryParam("param3", "value3")
                        // .queryParam("param4", "value4")
                        // .queryParam("param5", "value5")
                        .build())
                .header("Content-Type", "application/json")
                .retrieve()
                .toBodilessEntity();
        log.info("RESPONSE - " + response);

    }

    public void ast_db_put(String familyname, String key, String val) {
        DbPutAction putAction = new DbPutAction(familyname, key, val);
        try {
            asteriskInfraPort.sendAction(putAction);
        } catch (Exception ex) {
            log.error(ex.getStackTrace().toString());
        }

    }

    @Override
    public void dialBeginEvent(DialBeginEvent newevent) {
        // TODO Auto-generated method stub
        log.debug("Received a dialBeginEvent: {}", newevent);
        HashMap<String, String> currentChannelVars = getChannelVars(newevent.getUniqueId());
        updateLinkedChannels(newevent.getUniqueId(), newevent.getDestUniqueId());
        if (currentChannelVars == null) {
            return;
        }
        // System.out.println("Current channel Vars: " + currentChannelVars);
        // if (ast_channel_cdr(chan)) {
        // struct ast_var_t *current;
        // struct ast_var_t *newvar;
        String varname;
        int vartype;

        // AST_LIST_TRAVERSE(ast_channel_varshead(chan), current, entries) {
        // ast_vitel_log("VARS: ", ast_var_full_name(current), ast_var_value(current),
        // "TEST", "TEST", "TEST", "", "","");
        // }
        // ast_vitel_log(ast_channel_caller(chan)->id.number.str,
        // ast_channel_dialed(chan)->number.str, exten, "TEST", "TEST", "TEST", "",
        // "","");
        String trExten = currentChannelVars.get("BLINDTRANSFER");
        String txDst = currentChannelVars.get("MANUAL_BLIND_TRANSFER_DEST");
        String txFail = currentChannelVars.get("TXFAIL");
        String txNo = currentChannelVars.get("exten");
        String temp1 = currentChannelVars.get("VITELVARS");
        String temp2 = currentChannelVars.get("VITELVARS");// pbx_builtin_getvar_helper(tmp->chan, "VITELVARS");
        String cSrc = currentChannelVars.get("FROM");
        String v4func = currentChannelVars.get("V4FUNC");
        String vuserfield = currentChannelVars.get("vuserfield");
        String caller = currentChannelVars.get("SRC");
        String callee = currentChannelVars.get("EXTEN");
        String altcaller = currentChannelVars.get("ALTCALLER");
        String huntgroup = currentChannelVars.get("HUNTSTRATEGY");
        String numsubst = currentChannelVars.get("DIALLED_EXTEN");
        String huntgroupnumber = currentChannelVars.get("HUNTGROUPNUMBER");
        if (numsubst == null) {
            numsubst = "";
        }
        // Find the actual extension if non-local
        String dialledNum = getPeerName(numsubst);
        // if (dialledNum)
        // {
        // dialledNum++;
        // }
        // else
        // {
        // dialledNum = numsubst;
        // }
        // pbx_builtin_setvar_helper(tmp->chan, "__DIALLED_EXTEN", dialledNum);
        String calleridnum = newevent.getCallerIdNum();
        String exten = newevent.getDialString();// ast_channel_exten(chan)
        String sourceChannel = newevent.getChannel();// ast_channel_name(chan)
        String destChannel = newevent.getDestChannel();// ast_channel_name(tmp->chan)
        String uniqueidChannel = newevent.getUniqueId();// ast_channel_uniqueid(chan)
        // AsteriskChannel astChannel = getAstChannel(newevent.getUniqueId());
        // astChannel.setVariable("__DIALLED_EXTENtest", dialledNum);
        SetVarAction setvar = new SetVarAction(newevent.getDestChannel(), "__DIALLED_EXTEN", dialledNum);
        try {
            asteriskInfraPort.sendAction(setvar);
        } catch (Exception ex) {
            log.error(ex.getStackTrace().toString());
        }
        //// pbx_builtin_setvar_helper(chan, "DIALLED_EXTEN", dialledNum);
        setvar = new SetVarAction(newevent.getChannel(), "DIALLED_EXTENmctest", dialledNum);
        try {
            asteriskInfraPort.sendAction(setvar);
        } catch (Exception ex) {
            log.error(ex.getStackTrace().toString());
        }
        if (caller != null) {
            calleridnum = caller;
            // sprintf(ast_channel_caller(chan)->id.number.str, "%s", caller);
        }
        if (callee != null) {
            exten = callee;
            // sprintf(ast_channel_exten(chan), "%s", callee);
        }
        String callingVars = "NONE";
        String calledVars = "NONE";
        if (temp1 != null && strlen(temp1) > 0) {
            callingVars = temp1;
        }
        if (temp2 != null && strlen(temp2) > 0) {
            calledVars = temp2;
        }
        if (v4func == null) {
            v4func = "NONE";
        }
        // ast_channel_caller(chan)->id.number.str
        if (strcmp(v4func, "TRANSFER")) {
            // TODO
            if (altcaller != null) {
                // sprintf(ast_channel_caller(chan)->id.number.str, "%s", altcaller);
                calleridnum = altcaller;
            }
            // sprintf(ast_channel_exten(chan), "%s", txNo);
            exten = txNo;
            if (strcmp(txFail, "true")) {
                exten = trExten;
                // sprintf(ast_channel_exten(chan), "%s", trExten);
            }
            if (huntgroup != null) {
                ast_vitel_log(uniqueidChannel, "ORIGINATED", calleridnum, dialledNum, "HUNTGROUP",
                        sourceChannel, destChannel, ((huntgroupnumber != null) ? huntgroupnumber : exten), null,
                        null, null);
            } else {
                ast_vitel_log(uniqueidChannel, "ORIGINATED", calleridnum, exten, vuserfield, callingVars,
                        calledVars, sourceChannel, destChannel, trExten, txFail);
            }
        } else {
            if (cSrc != null && strcmp(cSrc, "QUEUE")) {
                // System.out.println("VITEL LOG: " + newevent.getUniqueId() + "|" +
                // "ORIGINATED" + "|" + calleridnum + "|" + exten + "|" + newevent.getChannel()
                // + "|" + newevent.getDestChannel() + "|" + callingVars + "|" + calledVars +
                // "|" + "QUEUE");
                ast_vitel_log(uniqueidChannel, "ORIGINATED", calleridnum, exten, sourceChannel, destChannel,
                        callingVars, calledVars, "QUEUE", null, null);
            } // TODO
            else {
                if (txDst != null) {
                    if (huntgroup != null) {
                        ast_vitel_log(uniqueidChannel, "ORIGINATED", calleridnum, dialledNum, "HUNTGROUP",
                                sourceChannel, destChannel,
                                ((huntgroupnumber != null) ? huntgroupnumber : exten), null, null, null);
                    } else {
                        ast_vitel_log(uniqueidChannel, "ORIGINATED", calleridnum, txDst, sourceChannel,
                                destChannel, callingVars, calledVars, null, null, null);
                    }
                } else {
                    if (huntgroup != null) {
                        ast_vitel_log(uniqueidChannel, "ORIGINATED", calleridnum, dialledNum, "HUNTGROUP",
                                sourceChannel, destChannel,
                                ((huntgroupnumber != null) ? huntgroupnumber : exten), null, null, null);
                    } else {
                        ast_vitel_log(uniqueidChannel, "ORIGINATED", calleridnum, exten, sourceChannel,
                                destChannel, callingVars, calledVars, null, null, null);
                    }
                }
            }
        }
    }

    @Override
    public void dialEndEvent(DialEndEvent newevent) {
        // TODO Auto-generated method stub
        log.debug("Received a dialEndEvent: {}", newevent);
        // <VITEL>
        HashMap<String, String> currentChannelVars = getChannelVars(newevent.getUniqueId());
        if (currentChannelVars == null) {
            return;
        }
        String suppress = currentChannelVars.get("SUPPRESSVLOG");
        if (suppress == null || !strcmp(suppress, "true")) {
            // if (ast_channel_cdr(in)) {
            String trExten = currentChannelVars.get("BLINDTRANSFER");
            String txDst = currentChannelVars.get("MANUAL_BLIND_TRANSFER_DEST");
            String txFail = currentChannelVars.get("TXFAIL");
            String txNo = currentChannelVars.get("exten");
            String temp1 = currentChannelVars.get("VITELVARS");
            String temp2 = currentChannelVars.get("VITELVARS");
            // TODO
            // String temp2 = pbx_builtin_getvar_helper(peer, "VITELVARS");
            String cSrc = currentChannelVars.get("FROM");
            String v4func = currentChannelVars.get("V4FUNC");
            String vuserfield = currentChannelVars.get("vuserfield");
            String huntgroup = currentChannelVars.get("HUNTSTRATEGY");
            String huntgroupnumber = currentChannelVars.get("HUNTGROUPNUMBER");
            String caller = currentChannelVars.get("SRC");
            String callee = currentChannelVars.get("EXTEN");
            String tempNo = null; // TODO pbx_builtin_getvar_helper(c, "DIALLED_EXTEN");
            String calleridnum = newevent.getCallerIdNum();
            String exten = newevent.getConnectedLineNum();// ast_channel_exten(chan)
            String sourceChannel = newevent.getChannel();// ast_channel_name(chan)
            String destChannel = newevent.getDestChannel();// ast_channel_name(tmp->chan)
            String uniqueidChannel = newevent.getUniqueId();// ast_channel_uniqueid(chan)

            if (tempNo == null) {
                tempNo = exten;
            }
            // Find the actual extension if non-local
            String dialledNum = getPeerName(tempNo);

            if (caller != null) {
                calleridnum = caller;
                // sprintf(ast_channel_caller(chan)->id.number.str, "%s", caller);
            }
            if (callee != null) {
                exten = callee;
                // sprintf(ast_channel_exten(chan), "%s", callee);
            }
            String callingVars = "NONE";
            String calledVars = "NONE";
            if (temp1 != null && strlen(temp1) > 0) {
                callingVars = temp1;
            }
            if (temp2 != null && strlen(temp2) > 0) {
                calledVars = temp2;
            }
            if (v4func == null) {
                v4func = "NONE";
            }

            if (strcmp(v4func, "TRANSFER")) {
                exten = txNo;
                if (strcmp(txFail, "true")) {
                    exten = trExten;
                }
                if (huntgroup != null) {
                    ast_vitel_log(uniqueidChannel, "ANSWERED", calleridnum, dialledNum, vuserfield,
                            "transferred", callingVars, calledVars,
                            sourceChannel,
                            destChannel, trExten, txFail, ((huntgroupnumber != null) ? huntgroupnumber
                                    : exten));
                } else {
                    ast_vitel_log(uniqueidChannel, "ANSWERED", calleridnum,
                            exten, vuserfield, "transferred", callingVars, calledVars,
                            sourceChannel,
                            destChannel, trExten, txFail);
                }
            } else {
                if (cSrc != null && strcmp(cSrc, "QUEUE")) {
                    ast_vitel_log(uniqueidChannel, "ANSWERED", calleridnum,
                            exten,
                            sourceChannel,
                            destChannel, callingVars, calledVars,
                            "QUEUE");
                } else {
                    if (txDst != null) {
                        if (huntgroup != null) {
                            ast_vitel_log(uniqueidChannel, "ANSWERED", calleridnum, dialledNum,
                                    "HUNTGROUP", sourceChannel,
                                    destChannel, ((huntgroupnumber != null) ? huntgroupnumber
                                            : exten),
                                    "");
                        } else {
                            ast_vitel_log(uniqueidChannel, "ANSWERED", calleridnum, txDst,
                                    sourceChannel,
                                    destChannel, callingVars, calledVars);
                        }
                    } else {
                        if (huntgroup != null) {
                            ast_vitel_log(uniqueidChannel, "ANSWERED", calleridnum, dialledNum,
                                    "HUNTGROUP", sourceChannel,
                                    destChannel, ((huntgroupnumber != null) ? huntgroupnumber
                                            : exten));
                        } else {
                            ast_vitel_log(uniqueidChannel, "ANSWERED", calleridnum,
                                    exten,
                                    sourceChannel,
                                    destChannel, callingVars, calledVars);
                        }
                    }
                }
            }
            // }
        }
        // </VITEL>
    }

    @Override
    public void monitorStartEvent(MonitorStartEvent newevent) {
        log.debug("Received a monitorStartEvent: {}", newevent);
        HashMap<String, String> currentChannelVars = getChannelVars(newevent.getUniqueId());
        for (String linkedid : getLinkedChannelsSet(newevent.getUniqueId())) {
            currentChannelVars = getChannelVars(linkedid);
            if (currentChannelVars != null) {
                break;
            }
        }
        if (currentChannelVars == null) {
            return;
        }
        String suppress = currentChannelVars.get("SUPPRESSVLOG");
        // newevent.get

        // //<VITEL>
        // const char *suppress = pbx_builtin_getvar_helper(chan, "SUPPRESSVLOGREC");
        if (suppress == null || !strcmp(suppress, "true")) {
            String baseExten = currentChannelVars.get("RECORDINGDEVICE");
            String UUIDtmp = currentChannelVars.get("RUID_TMP");
            String UUID = currentChannelVars.get("RUID");
            String voicemail_box = currentChannelVars.get("VOICEMAIL_BOX");
            String voicemail_type = currentChannelVars.get("VOICEMAIL_TYPE");
            String RECFILENAME = currentChannelVars.get("RECFILENAME");

            if (baseExten == null) {
                // baseExten = ast_channel_caller(chan)->id.number.str;
            }

            if (UUID == null && UUIDtmp != null) {
                UUID = UUIDtmp;
                SetVarAction setvar = new SetVarAction(newevent.getChannel(), "RUID", UUID);

                try {
                    asteriskInfraPort.sendAction(setvar);
                } catch (Exception ex) {
                    log.error(ex.getStackTrace().toString());
                }

                // pbx_builtin_setvar_helper(chan, "RUID", UUID);
            } else if (UUID == null && UUIDtmp == null) {
                UUID = "N/A";
            }
            if (voicemail_type == null) {
                voicemail_type = "UNKNOWN";
            }
            if (voicemail_box == null) {
                ast_vitel_log("NONE", "RECORD", baseExten, UUID,
                        newevent.getCallerIdNum(), "start",
                        RECFILENAME, newevent.getChannel());
                // ast_vitel_log(uniqueidChannel, "ORIGINATED", calleridnum, dialledNum,
                // "HUNTGROUP",
                // sourceChannel, destChannel,
                // ((huntgroupnumber != null) ? huntgroupnumber : exten), null, null, null);

            } else {
                ast_vitel_log("NONE", "RECORD", baseExten, UUID,
                        newevent.getCallerIdNum(), "start",
                        RECFILENAME, newevent.getChannel(), voicemail_type, voicemail_box);
            }
        }
        // </VITEL>
        // TODO Auto-generated method stub
        // throw new UnsupportedOperationException("Unimplemented method
        // 'monitorStartEvent'");
    }

    @Override
    public void monitorStopEvent(MonitorStopEvent newevent) {
        log.debug("Received a monitorStopEvent: {}", newevent);
        // TODO Auto-generated method stub
        // throw new UnsupportedOperationException("Unimplemented method
        // 'monitorStopEvent'");
        HashMap<String, String> currentChannelVars = getChannelVars(newevent.getUniqueId());
        for (String linkedid : getLinkedChannelsSet(newevent.getUniqueId())) {
            currentChannelVars = getChannelVars(linkedid);
            if (currentChannelVars != null) {
                break;
            }
        }
        if (currentChannelVars == null) {
            return;
        }
        String suppress = currentChannelVars.get("SUPPRESSVLOGREC");
        // newevent.get

        // //<VITEL>
        // const char *suppress = pbx_builtin_getvar_helper(chan, "SUPPRESSVLOGREC");
        if (suppress == null || !strcmp(suppress, "true")) {
            String baseExten = currentChannelVars.get("RECORDINGDEVICE");
            String UUIDtmp = currentChannelVars.get("RUID_TMP");
            String UUID = currentChannelVars.get("RUID");
            String voicemail_box = currentChannelVars.get("VOICEMAIL_BOX");
            String voicemail_type = currentChannelVars.get("VOICEMAIL_TYPE");
            String RECFILENAME = currentChannelVars.get("RECFILENAME");
            UUID = getUUIDfromRECFILENAME(RECFILENAME);
            System.out.println(UUID);
            if (baseExten == null) {
                // baseExten = ast_channel_caller(chan)->id.number.str;
            }

            if (UUID == null && UUIDtmp != null) {
                UUID = UUIDtmp;
                SetVarAction setvar = new SetVarAction(newevent.getChannel(), "RUID", UUID);

                try {
                    asteriskInfraPort.sendAction(setvar);
                } catch (Exception ex) {
                    log.error(ex.getStackTrace().toString());
                }

                // pbx_builtin_setvar_helper(chan, "RUID", UUID);
            } else if (UUID == null && UUIDtmp == null) {
                UUID = "N/A";
            }
            if (voicemail_type == null) {
                voicemail_type = "UNKNOWN";
            }
            if (voicemail_box == null) {
                // ast_vitel_log("NONE", "RECORD", baseExten, UUID,
                // ast_channel_caller(chan)->id.number.str, "stop",
                // ast_channel_monitor(chan)->filename_base, "|%s", ast_channel_name(chan));
                ast_vitel_log("NONE", "RECORD", baseExten, UUID,
                        newevent.getCallerIdNum(), "stop",
                        RECFILENAME, newevent.getChannel());
                // ast_vitel_log(uniqueidChannel, "ORIGINATED", calleridnum, dialledNum,
                // "HUNTGROUP",
                // sourceChannel, destChannel,
                // ((huntgroupnumber != null) ? huntgroupnumber : exten), null, null, null);

            } else {
                // ast_vitel_log("NONE", "RECORD", baseExten, UUID,
                // ast_channel_caller(chan)->id.number.str, "stop",
                // ast_channel_monitor(chan)->filename_base, "|%s|%s|%s",
                // ast_channel_name(chan), voicemail_type, voicemail_box);
                ast_vitel_log("NONE", "RECORD", baseExten, UUID,
                        newevent.getCallerIdNum(), "stop",
                        RECFILENAME, newevent.getChannel(), voicemail_type, voicemail_box);
            }
            // pbx_builtin_setvar_helper(chan, "RUID", NULL);
            // pbx_builtin_setvar_helper(chan, "RUID_TMP", NULL);
            // setvar = new SetVarAction(newevent.getChannel(), "DIALLED_EXTENmctest",
            // dialledNum);

        }
        // </VITEL>
        removeChannelVars(newevent.getUniqueId());
        removeLinkedChannelsMap(newevent.getUniqueId());
    }

    String getUUIDfromRECFILENAME(String inputString) {
        String[] split = inputString.split("-");
        String[] tempArray = new String[split.length - 3];
        // split.re
        // String uuid = java.util.Arrays.toString();

        for (int i = 0, k = 0; i < split.length; i++) {

            // if the index is
            // the removal element index
            if (i == 0 || i == 1 || i == 2) {
                continue;
            }

            // if the index is not
            // the removal element index
            tempArray[k++] = split[i];
        }
        return String.join("-", tempArray);

    }

    @Override
    public void hangupRequestEvent(HangupRequestEvent hangupRequestEvent) {
        // TODO Auto-generated method stub
        throw new UnsupportedOperationException("Unimplemented method 'hangupRequestEvent'");
    }
}
