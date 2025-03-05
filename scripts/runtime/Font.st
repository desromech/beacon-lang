Font class ![
initialize
    DejaVuSans := Font loadFontFromFile: __FileDir__ , 'fonts/DejaVuSans.ttf'.
    DejaVuSansSmallFace := DejaVuSans createFaceWithHeight: 16.
    DejaVuSansSmallHiDpiFace := DejaVuSans createFaceWithHeight: 32.
    DejaVuSansSmallFace hiDpiScaled: DejaVuSansSmallHiDpiFace.

    DejaVuSansMono := Font loadFontFromFile: __FileDir__ , 'fonts/DejaVuSansMono.ttf'.
    DejaVuSansMonoSmallFace := DejaVuSansMono createFaceWithHeight: 16.
    DejaVuSansMonoSmallHiDpiFace := DejaVuSansMono createFaceWithHeight: 32.
    DejaVuSansMonoSmallFace hiDpiScaled: DejaVuSansMonoSmallHiDpiFace.
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

Font class ![
defaultMonospacedFont
    ^ DejaVuSansMono
].

Font class ![
defaultMonospacedFontFace
    ^ DejaVuSansMonoSmallFace
].

Font class ![
defaultHiDpiMonospacedFontFace
    ^ DejaVuSansMonoSmallHiDpiFace
].

FontFace ![
height
    ^ height
].


FontFace ![
linespace
    ^ ascent - descent + linegap
].


FontFace ![
ascent
    ^ ascent
].

FontFace ![
descent
    ^ descent
].

FontFace ![
linegap
    ^ linegap
].

FontFace ![
hiDpiScaled
    ^ hiDpiScaled
].

FontFace ![
hiDpiScaled: aFontFace
    hiDpiScaled := aFontFace
].

Font initialize.