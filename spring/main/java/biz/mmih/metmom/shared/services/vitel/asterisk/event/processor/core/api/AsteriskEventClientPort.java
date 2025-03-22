package biz.mmih.metmom.shared.services.vitel.asterisk.event.processor.core.api;

import org.asteriskjava.manager.ManagerEventListener;
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

public interface AsteriskEventClientPort {

    void dialEvent(DialEvent dialEvent);

    void dialBeginEvent(DialBeginEvent dialEvent);

    void dialEndEvent(DialEndEvent dialEvent);

    void dialStateEvent(DialStateEvent dialStateEvent);

    void peerStatusEvent(PeerStatusEvent peerStatusEvent);

    void newChannelEvent(NewChannelEvent newChannelEvent);

    void newExtenEvent(NewExtenEvent newExtenEvent);

    void localBridgeEvent(LocalBridgeEvent localBridgeEvent);

    void fullyBootedEvent(FullyBootedEvent fullyBootedEvent);

    void successfulAuthEvent(SuccessfulAuthEvent successfulAuthEvent);

    void contactStatusEvent(ContactStatusEvent newEvent);

    void connectEvent(ConnectEvent connectEvent);

    void softHangupRequestEvent(SoftHangupRequestEvent softHangupRequestEvent);

    void hangupRequestEvent(HangupRequestEvent hangupRequestEvent);

    void hangupEvent(HangupEvent hangupEvent);

    void originateResponseEvent(OriginateResponseEvent originateResponseEvent);

    void deviceStateChangeEvent(DeviceStateChangeEvent deviceStateChangeEvent);

    void varSetEvent(VarSetEvent varSetEvent);

    void setEventListener(ManagerEventListener eventListener);

    void monitorStartEvent(MonitorStartEvent event);

    void monitorStopEvent(MonitorStopEvent event);

}
