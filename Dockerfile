FROM ubuntu AS builder

WORKDIR /app

RUN apt-get update && \
	DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends clang cmake git make ca-certificates wget

RUN mkdir /usr/local/share/ca-certificates/cacert.org && \
	wget -P /usr/local/share/ca-certificates/cacert.org http://www.cacert.org/certs/root.crt http://www.cacert.org/certs/class3.crt && \
	update-ca-certificates && \
	git config --global http.sslCAinfo /etc/ssl/certs/ca-certificates.crt

ADD . /app
RUN mkdir build && \
	cd build && \
	cmake .. && \
	make -j && \
	pwd && \
	make install

FROM ubuntu

RUN apt-get update && \
	apt-get install -y libgomp1 && \
	rm -rf /var/lib/apt/lists/*

COPY --from=builder /usr/local/etc/pfaedle /usr/local/etc/pfaedle
COPY --from=builder /usr/local/bin/pfaedle /usr/local/bin/pfaedle

ENTRYPOINT ["/bin/bash"]
