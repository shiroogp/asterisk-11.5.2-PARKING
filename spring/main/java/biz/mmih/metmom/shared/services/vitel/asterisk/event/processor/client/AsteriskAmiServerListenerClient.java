package biz.mmih.metmom.shared.services.vitel.asterisk.event.processor.client;

import biz.mmih.metmom.shared.services.vitel.asterisk.event.processor.core.api.AsteriskServerClientPort;
import lombok.extern.slf4j.Slf4j;
import org.asteriskjava.live.AbstractAsteriskServerListener;
import org.asteriskjava.live.AsteriskAgent;
import org.asteriskjava.live.AsteriskChannel;
import org.asteriskjava.live.AsteriskQueueEntry;
import org.asteriskjava.live.MeetMeUser;
import org.springframework.stereotype.Component;
import org.springframework.stereotype.Service;

@Slf4j
@Component
public class AsteriskAmiServerListenerClient extends AbstractAsteriskServerListener {

    private final AsteriskServerClientPort asteriskServerClientPort;

    public AsteriskAmiServerListenerClient(AsteriskServerClientPort asteriskServerClientPort) {
        this.asteriskServerClientPort = asteriskServerClientPort;
        this.asteriskServerClientPort.setServerListener(this);
    }

    @Override
    public void onNewAsteriskChannel(AsteriskChannel channel) {
        super.onNewAsteriskChannel(channel);
        asteriskServerClientPort.onNewAsteriskChannel(channel);
    }

    @Override
    public void onNewAgent(AsteriskAgent asteriskAgent) {
        asteriskServerClientPort.onNewAgent(asteriskAgent);
    }

    @Override
    public void onNewQueueEntry(AsteriskQueueEntry asteriskQueueEntry) {
        asteriskServerClientPort.onNewQueueEntry(asteriskQueueEntry);
    }

    @Override
    public void onNewMeetMeUser(MeetMeUser user) {
        super.onNewMeetMeUser(user);
        asteriskServerClientPort.onNewMeetMeUser(user);
    }
}