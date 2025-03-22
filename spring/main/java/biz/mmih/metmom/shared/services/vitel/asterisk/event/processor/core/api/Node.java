package biz.mmih.metmom.shared.services.vitel.asterisk.event.processor.core.api;

import org.springframework.boot.context.properties.ConfigurationProperties;
import org.springframework.context.annotation.Configuration;

import lombok.Getter;
import lombok.Setter;

@Getter
@Setter
// @Configuration
@ConfigurationProperties(prefix = "api.host")
@Configuration
public class Node {

    private String baseurl;

    // Getters and Setters go here
}