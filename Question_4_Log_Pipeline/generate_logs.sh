#!/bin/bash
# generate_logs.sh
# Simulates a live system by appending new log entries to app.log
# with short delays between each write. This is what makes
# `tail -f app.log` meaningful to demonstrate in the pipeline.

LOGFILE="app.log"

append_log () {
    echo "$(date '+%Y-%m-%d %H:%M:%S') $1" >> "$LOGFILE"
    sleep 1
}

append_log "INFO  Handling GET /api/products request"
append_log "ERROR Payment gateway timeout: no response from upstream"
append_log "WARN  Slow query detected: SELECT * FROM orders (2.3s)"
append_log "INFO  Cache refreshed for /api/products"
append_log "ERROR Redis connection refused: 127.0.0.1:6379"
append_log "INFO  Handling DELETE /api/session/882 request"
append_log "WARN  Certificate expires in 10 days"
append_log "ERROR Unhandled exception in OrderController.process(): NullPointerException"

echo "generate_logs.sh: finished appending 8 new log lines to $LOGFILE"
