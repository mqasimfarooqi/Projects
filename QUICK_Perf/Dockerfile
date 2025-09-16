# Use Eclipse Temurin JDK 21
FROM eclipse-temurin:21-jdk-jammy

WORKDIR /app

# Copy the built jar
COPY target/quic-app-0.1.0.jar /app/quic-app.jar

# Run as non-root user for safety
RUN useradd -m quicuser
USER quicuser

# Default command (server mode, but can be overridden by docker-compose)
CMD ["java", "-jar", "/app/quic-app.jar"]
