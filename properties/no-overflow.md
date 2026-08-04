It can never happen that the resulting type of an operation
is a signed-integer type but the resulting value is not in the
range of values that are representable by that type.
A violation of this property matches what C11 defines as undefined behavior.
(Hence, conversions to signed-integer types do not violate this property.)