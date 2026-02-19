# SPDX-License-Identifier: LicenseRef-FNCL-1.1
# Copyright (c) 2026 Christopher Gleiche
idf_component_register(
    SRCS "webui_dummy.c"
    INCLUDE_DIRS "."
    EMBED_TXTFILES
        "www/index.html"
        "www/app.js"
        "www/styles.css"
)
