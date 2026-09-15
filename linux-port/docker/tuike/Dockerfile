FROM python:3.13-slim

WORKDIR /app

# Dependencies first: the 16 MB of game artwork below changes far more often
# than these three lines, and this way a rebuild reuses the installed layer.
COPY requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt

COPY wsgi.py collector.py grants_worker.py ./
COPY VERSION CHANGELOG.md gm_commands.txt web_admin.quest ./
COPY tuike ./tuike

ENV PYTHONUNBUFFERED=1 \
    PYTHONDONTWRITEBYTECODE=1 \
    TUIKE_PANEL_PORT=7799

EXPOSE 7799

# Two workers so a slow query cannot block the page, four threads each because
# almost every request is waiting on MySQL rather than on the CPU.
CMD ["gunicorn", "--bind", "0.0.0.0:7799", "--workers", "2", "--threads", "4", \
     "--access-logfile", "-", "--error-logfile", "-", "wsgi:app"]
