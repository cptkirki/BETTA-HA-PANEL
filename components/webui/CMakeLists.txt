idf_component_register(
    SRCS "webui_dummy.c"
    INCLUDE_DIRS "."
    EMBED_TXTFILES
        "www/index.html"
        "www/app.js"
        "www/styles.css"
)
