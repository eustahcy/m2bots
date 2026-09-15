"""Gunicorn's entry point: `gunicorn wsgi:app`.

Running this file directly starts Flask's development server instead, which is
handy for looking at the panel without building an image.
"""
from tuike import create_app
from tuike import config

app = create_app()

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=config.PANEL_PORT, debug=True)
