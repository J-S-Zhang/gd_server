FROM ubuntu:22.04 AS builder

RUN apt-get update && apt-get install -y \
    build-essential cmake g++ \
    libboost-all-dev \
    libpq-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . .
RUN mkdir build && cd build \
    && cmake -DGUANDAN_ENABLE_BOOST=ON -DGUANDAN_USE_POSTGRES=ON .. \
    && cmake --build . --config Release

FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    libboost-system1.74.0 \
    libpq5 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY --from=builder /app/build/guandan_server .
COPY config/ ./config/
COPY config/database.json.example ./config/database.json

EXPOSE 9001 8080
CMD ["./guandan_server", "9001", "8080", "users.json", "config/app_version.json", "downloads/avatars", "config/livekit.json", "config/database.json"]
