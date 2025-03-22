package biz.mmih.metmom.shared.services.vitel.asterisk.event.processor.core.api;

import org.asteriskjava.live.AsteriskAgent;
import org.asteriskjava.live.AsteriskChannel;
import org.asteriskjava.live.AsteriskQueueEntry;
import org.asteriskjava.live.AsteriskServerListener;
import org.asteriskjava.live.MeetMeUser;

public interface AsteriskServerClientPort {

    void setServerListener(AsteriskServerListener serverListenerListener);

    void onNewAsteriskChannel(AsteriskChannel channel);

    void onNewAgent(AsteriskAgent asteriskAgent);

    void onNewQueueEntry(AsteriskQueueEntry asteriskQueueEntry);

    void onNewMeetMeUser(MeetMeUser user);
}
