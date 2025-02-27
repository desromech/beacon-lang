Integer ![
anyMask: mask
    ^ 0 ~= (self & mask)
].

Integer ![
allMask: mask
    ^ mask = (self & mask)
].
