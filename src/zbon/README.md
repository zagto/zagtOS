# Overview
ZBON (Zagtos Binary Object Notation) is a data format for Message Passing and other places where data needs to be serialized. It has feautres are similar to JSON, but the following major (performance-orientated) differences:

- It's not a plain text format, but binary (hence the name). A simple editor for ZBON data can be fould [here](../toolchain/ZBONEditor).

- ZBON is not build around key-value maps, it's "Objects" are only (ordered) values. A schema is used to make sense of the values. This makes the data much smaller, but also makes the format less flexible when in some ways: JSON applications can simply reject objects with additional properties not in the schama, ZBON has to be strict and reject these.

- Different (fixed-size) Number types exist.


# Application usage

## C++
A simple header-only implementation of ZBON is included in this repo: [zbon.hpp](zbon.hpp). Include it in your project.

If you use Zagtos, this library is already installed. Use the following to include it:

```#include <zbon/zbon.hpp>```

Otherwise, adjust the path to wherever you put the file.


### Notation

Many ZBON functions are overloaded. This document will use `<...>` for "put your favorite data type there".


### Encoding

```uint8_t *zbon::encode(<...>)```

This will allocate a buffer. It is your responsibility to free that buffer later.


### Decoding

```<...> zbon::decode(const uint8_t *buffer, size_t size, bool &success)```

Creates C++ object(s) form the ZBON data in buffer. Does not modify or deallocate the buffer. The value in `success` is set to `false` if the data in the buffer is malformed or does not match the schema defined by the result type, otherwise `true`.


### Supported data types by Default

- Booleans: `bool`
- Integers: `(u)int_(8/16/32/64)_t`. Make sure to use a type of fixed bit-width for compatibility across 32/64-bit systems.
- Floats: `float` and `double`
- Arrays: `std::vector`, `std::array` and C arrays of fixed size
- Strings: `std::string`. (currently assumed to be UTF-8, feel free to add proper conversion support for systems that use something else)


### Adding Support to own Classes

ZBON can handle classes that implement the following 3 things:


- `static zbon::Schema ZBONSchema`
  
  should hold the [Schema](#schemas) of the objects.


- `void toZBON(zbon::Encoder &encoder)`
  
  should instruct the Encoder to do the encoding of the properties. Use the given `encoder` argument call `encoder.encodeObjectProperty(<...>)` on anything that should be put as a property into the ZBON object.
  
  This method will actually be called multiple times when an object is encodes once, because the Encoder uses separate passes to calculate the size of objects and to actually encode them for performace reasons.
  
- `static <...> fromZBON(zbon::Decoder &decoder)`
    
  This method will be called on decode. Use the `decoder` given as an argument to call `decoder.decodeObjectProperty()` for each property in the order they are in the ZBON object.
  
  `decodeObjectProperty()` returns the value. If the next property is an Array or a String, you also may use `decoder.getArraySize()` to get the number of elements, use that to allocate a buffer the way you like and then use `decodeObjectProperty(ElementType *buffer, size_t numElements)` to decode there.


# Specification


## Alignment
Everything is 1-byte aligned, there are no gaps.


## Types
Type information is encoded as 1-byte values:

Value | Meaning
------|--------
0  | Object
1  | Nothing - used to encode that an optional value is not there
2  | Array
3  | String (UTF-8 encoded!)
4  | Boolean
5  | 8-bit signed Integer
6  | 8-bit unsigned Integer
7  | 16-bit signed Integer
8  | 16-bit unsigned Integer
9  | 32-bit signed Integer
10 | 32-bit unsigned Integer
11 | 64-bit signed Integer
12 | 64-bit unsigned Integer
13 | Single-Precision Float
14 | Double-Precision Float


## Sizes
Byte sizes, Element counts, etc. are always a 64-bit unsigned Integer.

Byte Sizes of an Element always include sub-Elements.


## Header

A ZBON File begins with one byte to specify the type of the root element. This is followed by the size in Bytes of the root element (which is also the size of the rest of the File - all non-Root elements are contained in the Root - a lot like JSON).

Offset | Size | Description
-------|------|-------------
0 | 1 | Type of Root Element
1 | 8 | rootSize - Size in Bytes of Root Element
9 | rootSize | Root Element


## Booleans

A single byte that is either 0 for `false` or 1 for `true`


## Integers

Little-Endian representation of differens bit-widths (8, 16, 32, 64). The bit-width is part of meta information and not encoded in the Integer itself. The same is true for the whether the value is signed or unsigned. Twos-Complement is used for negative numbers.


## Floats

Litte-Endian IEEE 754 representation. 

- Single Precision: 4 Bytes

- Double Precision: 8 Bytes


## Arrays

Offset | Size | Description
-------|------|-------------
0 | 1 | Type of the Elements
1 | 8 | numElements | Number of Elements
9 | 8 | bytesSize - Sum of the Sizes of all Elements in Bytes
17 | bytesSize | Elements


## Strings

Offset | Size | Description
-------|------|-------------
0 | 8 | bytesSize - Size in bytes of the String
8 | bytesSize | UTF-8 encoded String Data. There is no terminating 0 Byte.


## Objects


Offset | Size | Description
-------|------|-------------
0 | 8 | numberOfProperties - how many Properties does this Object have?
8 | 8 | bytesSize - Sum of the Sizes of all Properties in Bytes. Does not include the Type Byte at the beginning of each Property.
16 | 1 | Type of first Property
17 | depends | First Property
depends | 1 | Type of second Property
depends | depends | Second Property
... | ... | ...

Objects with 0 Properties are allowed. In this case numberOfProperties and bytesSize will be 0

