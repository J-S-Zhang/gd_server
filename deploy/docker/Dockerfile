FROM ubuntu:22.04 AS builder

RUN apt-get update && apt-get install -y \
    build-essential cmake g++ \
    libboost-all-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . .
RUN mkdir build && cd build \
    && cmake -DGUANDAN_ENABLE_BOOST=ON .. \
    && cmake --build . --config Release

FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    libboost-system1.74.0 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY --from=builder /app/build/guandan_server .
COPY config/ ./config/

EXPOSE 9001
CMD ["./guandan_server", "9001"]
