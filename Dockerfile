FROM gcc:13 AS builder

WORKDIR /app

COPY . .

RUN make

FROM debian:bookworm-slim

WORKDIR /app

RUN apt-get update && apt-get install -y --no-install-recommends libstdc++6 && rm -rf /var/lib/apt/lists/*

COPY --from=builder /app/leetcode_db .
COPY --from=builder /app/frontend ./frontend

EXPOSE 8000

CMD ["./leetcode_db"]
