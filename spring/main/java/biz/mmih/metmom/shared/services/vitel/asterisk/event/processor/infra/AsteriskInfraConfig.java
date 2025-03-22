package biz.mmih.metmom.shared.services.vitel.asterisk.event.processor.infra;

import lombok.Getter;
import lombok.Setter;
import org.asteriskjava.live.AsteriskServer;
import org.asteriskjava.live.internal.AsteriskServerImpl;
import org.asteriskjava.manager.ManagerConnection;
import org.asteriskjava.manager.ManagerConnectionFactory;
import org.springframework.boot.context.properties.ConfigurationProperties;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;

import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

@Getter
@Setter
@Configuration
@ConfigurationProperties(prefix = "app-config-asterisk-infra")
class AsteriskInfraConfig {

    private String host;
    private String user;
    private int port;
    private String password;

    @Bean
    ManagerConnectionFactory managerConnectionFactory() {
        return new ManagerConnectionFactory(host, port, user, password);
    }

    @Bean("managerConnection")
    ManagerConnection manageConnection() {
        return managerConnectionFactory().createManagerConnection();
    }

    @Bean
    ExecutorService executorService() {
        return Executors.newFixedThreadPool(1);
    }

    @Bean
    AsteriskServer asteriskServer() {
        return new AsteriskServerImpl(manageConnection());
    }
}
