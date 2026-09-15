"""Tuike - a control centre for a Metin2 world running Playerbots.

Created with create_app(). Everything the templates need that is not view data
is attached in one context processor below, so no view has to remember to pass
the brand, the theme or the icon helpers.
"""
from datetime import timedelta
from urllib.parse import quote

import pymysql
from flask import Flask, render_template, url_for
from markupsafe import escape

from . import config, db, settings
from .gamedata import characters as chardata
from .gamedata import items as itemdata
from .gamedata.maps import MAP_BACKGROUNDS, map_aspect, map_name
from .security import csrf_token, is_admin
from .text import hours, thousands


def create_app():
    app = Flask(__name__)
    app.secret_key = config.SESSION_SECRET
    app.config.update(
        SESSION_COOKIE_HTTPONLY=True,
        SESSION_COOKIE_SAMESITE="Lax",
        SESSION_COOKIE_NAME=config.SESSION_COOKIE_NAME,
        PERMANENT_SESSION_LIFETIME=timedelta(days=30),
        # Templates and the JSON views both benefit; the page is Polish and
        # sorting keys would only churn the diffs.
        JSON_SORT_KEYS=False,
        JSON_AS_ASCII=False,
    )

    register_template_globals(app)
    register_error_handlers(app)

    from .views import register_blueprints
    register_blueprints(app)
    return app


def register_template_globals(app):
    @app.context_processor
    def template_globals():
        current = settings.read()

        def item_icon(vnum):
            icon = itemdata.icon_file(vnum)
            return url_for("static", filename=f"icons/{quote(icon)}") if icon else None

        def class_portrait(job):
            return url_for("static", filename=f"class-portraits/{chardata.class_profile(job)['portrait']}")

        def empire_flag(empire):
            flag = chardata.empire_info(empire)["flag"]
            return url_for("static", filename=f"empires/{flag}") if flag else ""

        def map_background(index):
            picture = MAP_BACKGROUNDS.get(int(index or 0))
            return url_for("static", filename=f"maps/{picture}") if picture else ""

        return {
            "settings": current,
            "panel_brand": current.get("panel_name", "Metin2 Singleplayer"),
            "panel_codename": config.PANEL_CODENAME,
            "panel_tagline": config.PANEL_TAGLINE,
            "panel_version": config.PANEL_VERSION,
            "tieru_url": config.TIERU_PANEL_URL,
            "seban_url": config.SEBAN_PANEL_URL,
            "itemshop_url": config.ITEMSHOP_URL,
            "vps_panel_url": config.VPS_PANEL_URL,
            "is_admin": is_admin(),
            "csrf_token": csrf_token,
            # Game-data helpers, so templates never import a module.
            "map_name": map_name,
            "map_aspect": map_aspect,
            "map_background": map_background,
            "item_icon": item_icon,
            "job_name": chardata.job_name,
            "class_profile": chardata.class_profile,
            "class_portrait": class_portrait,
            "empire_info": chardata.empire_info,
            "empire_flag": empire_flag,
            "thousands": thousands,
            "hours": hours,
        }


def register_error_handlers(app):
    @app.errorhandler(pymysql.err.OperationalError)
    def crashed_table(error):
        """Say which table broke and how to repair it, rather than showing 500.

        MyISAM does not survive an unclean stop - a Docker shutdown mid-write
        or a power cut is enough - and this panel reads log.log on its front
        page. Updating the server does not fix it: the damage is in the data on
        the volume, not in the program.
        """
        if not db.is_crashed_table(error):
            raise error
        table = db.crashed_table_name(error)
        return render_template(
            "errors/crashed_table.html",
            table=table,
            errno=error.args[0] if error.args else 0,
            detail=escape(str(error.args[1]) if len(error.args) > 1 else str(error)),
        ), 500

    @app.errorhandler(403)
    def forbidden(error):
        return render_template("errors/message.html", title="Odmowa dostępu",
                               message=getattr(error, "description", "Brak uprawnień.")), 403

    @app.errorhandler(404)
    def not_found(error):
        return render_template("errors/message.html", title="Nie znaleziono",
                               message="Tej strony albo tego rekordu nie ma w tym świecie."), 404

    @app.errorhandler(400)
    def bad_request(error):
        return render_template("errors/message.html", title="Nieprawidłowe żądanie",
                               message=getattr(error, "description", "Sprawdź formularz.")), 400
