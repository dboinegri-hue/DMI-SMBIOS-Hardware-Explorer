# ── Stage 1: compile the C binary ─────────────────────────────────────────
FROM ubuntu:22.04 AS builder

RUN apt-get update \
 && apt-get install -y --no-install-recommends build-essential \
 && rm -rf /var/lib/apt/lists/*

WORKDIR /build
COPY src/ ./src/
COPY Makefile .
RUN make

# ── Stage 2: Python / Django runtime ──────────────────────────────────────
FROM python:3.12-slim

WORKDIR /app

# Install Django first (layer is cached unless requirements change)
COPY web/requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt

# Copy the web app and the compiled binary from stage 1
COPY web/ ./web/
COPY --from=builder /build/dmi_explorer .

EXPOSE 8000

# Django must listen on 0.0.0.0 so it's reachable outside the container
WORKDIR /app/web
CMD ["python3", "manage.py", "runserver", "0.0.0.0:8000"]
