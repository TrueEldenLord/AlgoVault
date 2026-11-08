FROM gcc:13 AS builder

WORKDIR /app

COPY . .

RUN make

FROM debian:bookworm-slim

WORKDIR /app

COPY --from=builder /app/leetcode_db .
COPY --from=builder /app/frontend ./frontend

EXPOSE 8000

CMD ["./leetcode_db"]
