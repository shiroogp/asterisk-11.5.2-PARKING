package biz.mmih.metmom.shared.services.vitel.asterisk.event.processor;

import jakarta.annotation.PreDestroy;
import lombok.extern.slf4j.Slf4j;
import org.asteriskjava.live.AsteriskServer;
import org.asteriskjava.manager.AuthenticationFailedException;
import org.asteriskjava.manager.ManagerConnectionState;
import org.asteriskjava.manager.TimeoutException;
import org.springframework.boot.CommandLineRunner;
import org.springframework.boot.SpringApplication;
import org.springframework.boot.autoconfigure.SpringBootApplication;

import java.io.IOException;

@SpringBootApplication
@Slf4j
public class AsteriskEventProcessorApplication implements CommandLineRunner {

    private final AsteriskServer asteriskServer;

    public AsteriskEventProcessorApplication(AsteriskServer asteriskServer) {
        this.asteriskServer = asteriskServer;
    }

    public static void main(String[] args) {
        SpringApplication
                .run(AsteriskEventProcessorApplication.class, args);
    }

    @Override
    public void run(String... args) {
        logManagerStatus(asteriskServer.getManagerConnection().getState());
    }

    @PreDestroy
    public void destroy() {
        log.info("Manager connection state {}, Trying... shutdown ", asteriskServer.getManagerConnection().getState());
        asteriskServer.shutdown();
        logManagerStatus(asteriskServer.getManagerConnection().getState());
    }

    private void logManagerStatus(ManagerConnectionState state) {
        log.info("Manager connection state {}", state);
    }
}
