Object subclass: #FormRenderingElementBuilder instanceVariables: #(scaleFactorX scaleFactorY renderingElements).

FormRenderingElementBuilder ![
initialize
    renderingElements := ArrayList new.
    scaleFactorX := 1.
    scaleFactorY := 1.
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
        rectangle: (rectangle scaledWithXFactor: scaleFactorX y: scaleFactorY);
        color: color;
        borderSize: borderSize;
        borderColor: borderColor;
        yourself.
    renderingElements add: solidRectangle.
    solidRectangle
].

FormRenderingElementBuilder ![
solidRectangle: rectangle color: color
    ^ self solidRectangle: rectangle color: color borderSize: 0 borderColor: Color black
].