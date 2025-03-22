package biz.mmih.metmom.shared.services.vitel.asterisk.event.processor.core.impl;

import biz.mmih.metmom.shared.services.vitel.asterisk.event.processor.core.api.AsteriskInfraPort;
import biz.mmih.metmom.shared.services.vitel.asterisk.event.processor.core.api.AsteriskServerClientPort;
import lombok.extern.slf4j.Slf4j;

import java.beans.PropertyChangeEvent;
import java.beans.PropertyChangeListener;
import java.util.Map;

import org.asteriskjava.live.AsteriskAgent;
import org.asteriskjava.live.AsteriskChannel;
import org.asteriskjava.live.AsteriskQueueEntry;
import org.asteriskjava.live.AsteriskServerListener;
import org.asteriskjava.live.MeetMeUser;
import org.springframework.stereotype.Service;

@Slf4j
@Service
class AsteriskServerClientAdaptor implements AsteriskServerClientPort, PropertyChangeListener {

    private final AsteriskInfraPort asteriskInfraPort;

    AsteriskServerClientAdaptor(AsteriskInfraPort asteriskInfraPort) {
        this.asteriskInfraPort = asteriskInfraPort;
    }

    @Override
    public void setServerListener(AsteriskServerListener serverListenerListener) {
        asteriskInfraPort.setAsteriskServerListener(serverListenerListener);
    }

    @Override
    public void onNewAsteriskChannel(AsteriskChannel event) {
        if (event.toString().toUpperCase().contains("PING") || event.toString().toUpperCase().contains("PONG")
                || event.toString().toUpperCase().contains("RTCPRECEIVEDEVENT")
                || event.toString().toUpperCase().contains("RTCPSENTEVENT")) {
            return;
        }
        // updateAsteriskChannels(event.getId(), event);

        System.out.println("Live server: " + event);
        event.addPropertyChangeListener(this);
        Map<String, String> variables = event.getVariables();

        // System.out.println("Live server Channel Variables: " + variables);
        // try {
        // System.out.println("Live server Dialed Channel Variables: " +
        // event.getDialedChannel().getVariables());
        //
        // } catch (Exception e) {
        // System.err.println("Live server Dialed Channel Variables: ");
        // }
        // try {
        // System.out.println("Live server Linked Channel Variables: " +
        // event.getLinkedChannel().getVariables());
        //
        // } catch (Exception e) {
        // System.err.println("Live server Dialed Channel Variables: ");
        // }
        // try {
        // System.out.println("Live server Dialing Channel Variables: " +
        // event.getDialingChannel().getVariables());
        //
        // } catch (Exception e) {
        // System.err.println("Live server Dialed Channel Variables: ");
        // }
    } // public void onNewAsteriskChannel(AsteriskChannel channel) {
      // log.debug("Received a newAsteriskChannel: {}", channel);
      // }

    @Override
    public void onNewAgent(AsteriskAgent asteriskAgent) {
        log.info("Received a newAgent: {}", asteriskAgent);
    }

    @Override
    public void onNewQueueEntry(AsteriskQueueEntry asteriskQueueEntry) {
        log.info("Received a newQueueEntry: {}", asteriskQueueEntry);
    }

    @Override
    public void onNewMeetMeUser(MeetMeUser user) {
        log.info("Received a newMeetMeUser: {}", user);
    }

    public void propertyChange(PropertyChangeEvent propertyChangeEvent) {
        System.out.println("Live server - prop change: " + propertyChangeEvent);

    }

}
