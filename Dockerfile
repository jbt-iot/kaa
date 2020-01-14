FROM openjdk:8

COPY server/node/target/kaa-node.deb .
RUN dpkg -i /kaa-node.deb
COPY server/node/target/sdk/cpp/kaa-cpp-ep-sdk-0.9.0.tar.gz /usr/lib/kaa-node/sdk/cpp/kaa-cpp-ep-sdk-0.9.0.tar.gz
COPY docker-nix/configs/* /etc/kaa-node/conf/
RUN ln -sf /dev/stdout /var/log/kaa/kaa-node.log
CMD sleep 120; \
java -cp /usr/lib/kaa-node/conf:/usr/lib/kaa-node/lib/* \
-Xms1G \
-Xmx4G \
-XX:+UseG1GC \
-agentlib:jdwp=transport=dt_socket,server=y,suspend=n,address=5005 \
-Dfile.encoding=UTF8 \
-Dserver_log_dir=/var/log/kaa \
-Dserver_log_sufix= \
-Dserver_home_dir=/usr/lib/kaa-node \
-Dcom.sun.management.jmxremote=true \
-Dcom.sun.management.jmxremote.port=7091 \
-Dcom.sun.management.jmxremote.authenticate=false \
-Dcom.sun.management.jmxremote.ssl=false org.kaaproject.kaa.server.node.KaaNodeApplication
