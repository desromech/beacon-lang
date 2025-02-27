Object subclass: #FormRenderingElementBuilder instanceVariables: #(scaleFactorX scaleFactorY translation renderingElements).

FormRenderingElementBuilder ![
initialize
    renderingElements := ArrayList new.
    scaleFactorX := 1.
    scaleFactorY := 1.
    translation := 0@0.
].

FormRenderingElementBuilder ![
scaleFactorX
    ^ scaleFactorX
].

FormRenderingElementBuilder ![
scaleFactorX: newScaleFactor
    scaleFactorX := newScaleFactor
].

FormRenderingElementBuilder ![
scaleFactorY
    ^ scaleFactorY
].

FormRenderingElementBuilder ![
scaleFactorY: newScaleFactor
    scaleFactorY := newScaleFactor
].

FormRenderingElementBuilder ![
translation
    ^ translation
].

FormRenderingElementBuilder ![
translation: newTranslation
    translation := newTranslation
].

FormRenderingElementBuilder ![
translatedBy: extraTranslation during: aBlock
    | oldTranslation |
    oldTranslation := translation.
    translation := translation + (extraTranslation * (scaleFactorX @ scaleFactorY)).
    
    "Use an ensure block here."
    aBlock value.
    
    translation := oldTranslation.
].


FormRenderingElement ![
borderRoundRadius
    ^ borderRoundRadius
].

FormRenderingElement ![
borderRoundRadius: aRadius
    borderRoundRadius := aRadius
].

FormRenderingElement ![
borderSize
    ^ borderSize
].

FormRenderingElement ![
borderSize: anInteger
    borderSize := anInteger
].

FormRenderingElement ![
rectangle
    ^ rectangle
].

FormRenderingElement ![
rectangle: aRectangle
    rectangle := aRectangle
].

FormSolidRectangleRenderingElement ![
color
    ^ color
].

FormSolidRectangleRenderingElement ![
color: aColor
    color := aColor
].

FormSolidRectangleRenderingElement ![
borderColor
    ^ borderColor
].

FormSolidRectangleRenderingElement ![
borderColor: aColor
    borderColor := aColor
].

FormTextRenderingElement ![
text
    ^ text
].

FormTextRenderingElement ![
text: aString
    text := aString
].

FormTextRenderingElement ![
color
    ^ color
].

FormTextRenderingElement ![
color: aColor
    color := aColor
].

FormTextRenderingElement ![
fontFace
    ^ fontFace
].

FormTextRenderingElement ![
fontFace: aFontFace
    fontFace := aFontFace
].

FormRenderingElementBuilder ![
drawInForm: aForm
    | elements |
    elements := renderingElements asArray.
    1 to: elements basicSize do: [:elementIndex |
        | element |
        element := elements basicAt: elementIndex.
        element drawInForm: aForm
    ]
].

FormRenderingElementBuilder ![
solidRectangle: rectangle color: color borderSize: borderSize borderColor: borderColor
    | solidRectangle |
    solidRectangle := FormSolidRectangleRenderingElement new
        rectangle: ((rectangle translatedBy: translation) scaledWithXFactor: scaleFactorX y: scaleFactorY);
        color: color;
        borderSize: borderSize*scaleFactorX;
        borderColor: borderColor;
        yourself.
    renderingElements add: solidRectangle.
    solidRectangle
].

FormRenderingElementBuilder ![
solidRectangle: rectangle color: color
    ^ self solidRectangle: rectangle color: color borderSize: 0 borderColor: Color black
].

FormRenderingElementBuilder ![
text: text rectangle: rectangle color: color fontFace: fontFace
    | textRectangle |
    textRectangle := FormTextRenderingElement new
        text: text;
        rectangle: rectangle;
        color: color;
        fontFace: fontFace
        yourself.
    renderingElements add: textRectangle.
    textRectangle
].

FormRenderingElementBuilder ![
text: text rectangle: rectangle color: color
    ^ self text: text rectangle: rectangle color: color fontFace: self defaultFontFace
].

FormRenderingElementBuilder ![
defaultFontFace
    ^ scaleFactorX >= 2
        ifTrue: [Font defaultHiDpiFontFace]
        ifFalse: [Font defaultFontFace]
].
