package biz.mmih.metmom.shared.services.vitel.asterisk.event.processor.client;

import biz.mmih.metmom.shared.services.vitel.asterisk.event.processor.core.api.AsteriskEventClientPort;
import lombok.extern.slf4j.Slf4j;
import org.asteriskjava.manager.AbstractManagerEventListener;
import org.asteriskjava.manager.ManagerEventListener;
import org.asteriskjava.manager.event.ConnectEvent;
import org.asteriskjava.manager.event.DeviceStateChangeEvent;
import org.asteriskjava.manager.event.DialBeginEvent;
import org.asteriskjava.manager.event.DialEndEvent;
import org.asteriskjava.manager.event.DialEvent;
import org.asteriskjava.manager.event.DialStateEvent;
import org.asteriskjava.manager.event.FullyBootedEvent;
import org.asteriskjava.manager.event.HangupEvent;
import org.asteriskjava.manager.event.HangupRequestEvent;
import org.asteriskjava.manager.event.LocalBridgeEvent;
import org.asteriskjava.manager.event.ManagerEvent;
import org.asteriskjava.manager.event.MonitorStartEvent;
import org.asteriskjava.manager.event.MonitorStopEvent;
import org.asteriskjava.manager.event.NewChannelEvent;
import org.asteriskjava.manager.event.NewExtenEvent;
import org.asteriskjava.manager.event.OriginateResponseEvent;
import org.asteriskjava.manager.event.PeerStatusEvent;
import org.asteriskjava.manager.event.SoftHangupRequestEvent;
import org.asteriskjava.manager.event.SuccessfulAuthEvent;
import org.asteriskjava.manager.event.ContactStatusEvent;
import org.asteriskjava.manager.event.VarSetEvent;
import org.springframework.stereotype.Component;
import org.springframework.stereotype.Service;

@Slf4j
@Component
public class AsteriskAmiEventListenerClient extends AbstractManagerEventListener implements ManagerEventListener {

    private final AsteriskEventClientPort asteriskEventClientPort;

    public AsteriskAmiEventListenerClient(AsteriskEventClientPort asteriskEventClientPort) {
        this.asteriskEventClientPort = asteriskEventClientPort;
        this.asteriskEventClientPort.setEventListener(this);
    }

    @Override
    public void handleEvent(DialEvent event) {
        super.handleEvent(event);
        asteriskEventClientPort.dialEvent(event);
    }

    @Override
    public void handleEvent(DialBeginEvent event) {
        super.handleEvent(event);
        asteriskEventClientPort.dialBeginEvent(event);
    }

    @Override
    public void handleEvent(DialEndEvent event) {
        super.handleEvent(event);
        asteriskEventClientPort.dialEndEvent(event);
    }

    @Override
    public void handleEvent(DialStateEvent event) {
        super.handleEvent(event);
        asteriskEventClientPort.dialStateEvent(event);
    }

    @Override
    public void handleEvent(PeerStatusEvent event) {
        // log.info("HANDLED peerstatusEvent {}", event.toString());
        super.handleEvent(event);
        asteriskEventClientPort.peerStatusEvent(event);
    }

    @Override
    public void handleEvent(NewChannelEvent event) {
        super.handleEvent(event);
        asteriskEventClientPort.newChannelEvent(event);

    }

    @Override
    public void handleEvent(NewExtenEvent event) {
        super.handleEvent(event);
        asteriskEventClientPort.newExtenEvent(event);

    }

    @Override
    public void handleEvent(LocalBridgeEvent event) {
        super.handleEvent(event);
        asteriskEventClientPort.localBridgeEvent(event);

    }

    @Override
    public void handleEvent(FullyBootedEvent event) {
        super.handleEvent(event);
        asteriskEventClientPort.fullyBootedEvent(event);

    }

    @Override
    public void handleEvent(SuccessfulAuthEvent event) {
        // log.info("HANDLED succesfulauthEvent {}", event.toString());
        super.handleEvent(event);
        asteriskEventClientPort.successfulAuthEvent(event);

    }

    @Override
    public void handleEvent(ContactStatusEvent event) {
        // log.info("HANDLED contactStatusEvent {}", event.toString());
        super.handleEvent(event);
        asteriskEventClientPort.contactStatusEvent(event);

    }

    @Override
    public void handleEvent(ConnectEvent event) {
        // log.info("HANDLED connectEvent {}", event.toString());
        super.handleEvent(event);
        asteriskEventClientPort.connectEvent(event);

    }

    @Override
    public void handleEvent(SoftHangupRequestEvent event) {
        super.handleEvent(event);
        asteriskEventClientPort.softHangupRequestEvent(event);

    }

    @Override
    public void handleEvent(HangupRequestEvent event) {
        super.handleEvent(event);
        asteriskEventClientPort.hangupRequestEvent(event);

    }

    @Override
    public void handleEvent(HangupEvent event) {
        super.handleEvent(event);
        asteriskEventClientPort.hangupEvent(event);

    }

    @Override
    public void handleEvent(OriginateResponseEvent event) {
        super.handleEvent(event);
        asteriskEventClientPort.originateResponseEvent(event);

    }

    @Override
    public void handleEvent(DeviceStateChangeEvent event) {
        super.handleEvent(event);
        asteriskEventClientPort.deviceStateChangeEvent(event);

    }

    @Override
    public void handleEvent(VarSetEvent event) {
        super.handleEvent(event);
        asteriskEventClientPort.varSetEvent(event);

    }

    @Override
    public void onManagerEvent(ManagerEvent event) {
        // log.info("HANDLED onManagerEvent {}", event.toString());
        super.onManagerEvent(event);
    }

    @Override
    public void handleEvent(MonitorStartEvent event) {
        super.handleEvent(event);
        asteriskEventClientPort.monitorStartEvent(event);
    }

    @Override
    public void handleEvent(MonitorStopEvent event) {
        super.handleEvent(event);
        asteriskEventClientPort.monitorStopEvent(event);
    }

}