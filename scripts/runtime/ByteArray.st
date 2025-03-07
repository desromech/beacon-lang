ByteArray ![
asByteArray
    ^ self
].

UInt16Array class ![
new: size
    ^ self basicNew: size * 2
].

UInt32Array class ![
new: size
    ^ self basicNew: size * 4
].

Float32Array class ![
new: size
    ^ self basicNew: size * 4
].

Float64Array class ![
new: size
    ^ self basicNew: size * 8
].
