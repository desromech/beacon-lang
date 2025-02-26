Font class ![
initialize
    DejaVuSans := Font loadFontFromFile: __FileDir__ , 'fonts/DejaVuSans.ttf'.
    DejaVuSansSmallFace := DejaVuSans createFaceWithHeight: 14.
    DejaVuSansSmallHiDpiFace := DejaVuSans createFaceWithHeight: 28.
].

Font class ![
dejaVuSans
    ^ DejaVuSans
].

Font class ![
dejaVuSansSmallFace
    ^ DejaVuSansSmallFace
].

Font class ![
DejaVuSansSmallHiDpiFace
    ^ DejaVuSansSmallHiDpiFace
].

Font class ![
defaultFont
    ^ DejaVuSans
].

Font class ![
defaultFontFace
    ^ DejaVuSansSmallFace
].

Font class ![
defaultHiDpiFontFace
    ^ DejaVuSansSmallHiDpiFace
].
Font initialize.
