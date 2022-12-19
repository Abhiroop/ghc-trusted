## 0.6.0

- Shipped with GHC 8.8.1

- Added to `GHC.Prim`:

        traceBinaryEvent# :: Addr# -> Int# -> State# s -> State# s

## 0.5.3 (edit as necessary)

- Shipped with GHC 8.6.1

- Added to `GHC.Prim`:

        addWordC# :: Word# -> Word# -> (# Word#, Int# #)

- `unpackClosure#` can now unpack any valid Haskell closure.
  Previously it returned empty pointer and non-pointer arrays
  for thunks.

- Add unaligned bytearray access primops (#4442)

         readWord8ArrayAsChar# :: MutableByteArray# s -> Int# -> State# s -> (# State# s, Char# #)
         readWord8ArrayAsAddr# :: MutableByteArray# s -> Int# -> State# s -> (# State# s, Addr# #)
         readWord8ArrayAsFloat# :: MutableByteArray# s -> Int# -> State# s -> (# State# s, Float# #)
         readWord8ArrayAsDouble# :: MutableByteArray# s -> Int# -> State# s -> (# State# s, Double# #)
         readWord8ArrayAsStablePtr# :: MutableByteArray# s -> Int# -> State# s -> (# State# s, StablePtr# #)
         readWord8ArrayAsInt16# :: MutableByteArray# s -> Int# -> State# s -> (# State# s, Int16# #)
         readWord8ArrayAsInt32# :: MutableByteArray# s -> Int# -> State# s -> (# State# s, Int32# #)
         readWord8ArrayAsInt64# :: MutableByteArray# s -> Int# -> State# s -> (# State# s, Int64# #)
         readWord8ArrayAsInt# :: MutableByteArray# s -> Int# -> State# s -> (# State# s, Int# #)

         readWord8ArrayAsWord16# :: MutableByteArray# s -> Int# -> State# s -> (# State# s, Word16# #)
         readWord8ArrayAsWord32# :: MutableByteArray# s -> Int# -> State# s -> (# State# s, Word32# #)
         readWord8ArrayAsWord64# :: MutableByteArray# s -> Int# -> State# s -> (# State# s, Word64# #)
         readWord8ArrayAsWord# :: MutableByteArray# s -> Int# -> State# s -> (# State# s, Word# #)

         writeWord8ArrayAsChar# :: MutableByteArray# s -> Int# -> Char# -> State# s -> State# s
         writeWord8ArrayAsAddr# :: MutableByteArray# s -> Int# -> Addr# -> State# s -> State# s
         writeWord8ArrayAsFloat# :: MutableByteArray# s -> Int# -> Float# -> State# s -> State# s
         writeWord8ArrayAsDouble# :: MutableByteArray# s -> Int# -> Double# -> State# s -> State# s
         writeWord8ArrayAsStablePtr# :: MutableByteArray# s -> Int# -> StablePtr# -> State# s -> State# s

         writeWord8ArrayAsInt16# :: MutableByteArray# s -> Int# -> Int16# -> State# s -> State# s
         writeWord8ArrayAsInt32# :: MutableByteArray# s -> Int# -> Int32# -> State# s -> State# s
         writeWord8ArrayAsInt64# :: MutableByteArray# s -> Int# -> Int64# -> State# s -> State# s
         writeWord8ArrayAsInt# :: MutableByteArray# s -> Int# -> Int# -> State# s -> State# s

         writeWord8ArrayAsWord16# :: MutableByteArray# s -> Int# -> Word16# -> State# s -> State# s
         writeWord8ArrayAsWord32# :: MutableByteArray# s -> Int# -> Word32# -> State# s -> State# s
         writeWord8ArrayAsWord64# :: MutableByteArray# s -> Int# -> Word64# -> State# s -> State# s
         writeWord8ArrayAsWord# :: MutableByteArray# s -> Int# -> Word# -> State# s -> State# s

## 0.5.2.0

- Shipped with GHC 8.4.1

- Added to `GHC.Prim`:

        compareByteArrays# :: ByteArray# -> Int# -> ByteArray# -> Int# -> Int# -> Int#

- Don't allocate a thunk for each unpacked UTF-8 character in `unpackCStringUtf8#`

## 0.5.1.1 *November 2017*

- Shipped with GHC 8.2.2

- Changed strictness properties of `catchRetry#` (#14171)

## 0.5.1.0 *July 2017*

- Shipped with GHC 8.2.1

- Added to `GHC.Prim`:

        fabsDouble# :: Double# -> Double#
        fabsFloat# :: Float# -> Float#
        isByteArrayPinned# :: ByteArray# -> Int#
        isMutableByteArrayPinned# :: MutableByteArray# s -> Int#
        anyToAddr# :: a -> State# (RealWorld) -> (# State# (RealWorld),Addr# #)

- New primitives for compact regions in `GHC.Prim`:

        Compact#
        compactNew#
        compactResize#
        compactContains#
        compactContainsAny#
        compactGetFirstBlock#
        compactGetNextBlock#
        compactAllocateBlock#
        compactFixupPointers#
        compactAdd#
        compactAddWithSharing#
        compactSize#

- Generalised `noDuplicate#` from

        noDuplicate# :: State# (RealWorld) -> State# (RealWorld)

    to

        noDuplicate# :: State# s -> State# s


## 0.5.0.0

- Shipped with GHC 8.0.1

- `GHC.Classes`: new `class IP (a :: Symbol) b | a -> b`

- `GHC.Prim`: changed type signatures from

        check# :: (State# RealWorld -> (# State# RealWorld, a #)) -> State# RealWorld -> (# State# RealWorld, () #)
        finalizeWeak# :: Weak# a -> State# RealWorld -> (# State# RealWorld, Int#, State# RealWorld -> (# State# RealWorld, () #) #)
        mkWeak# :: a -> b -> c -> State# RealWorld -> (# State# RealWorld, Weak# b #)

    to

        check# :: (State# RealWorld -> (# State# RealWorld, a #)) -> State# RealWorld -> State# RealWorld
        finalizeWeak# :: Weak# a -> State# RealWorld -> (# State# RealWorld, Int#, State# RealWorld -> (# State# RealWorld, b #) #)
        mkWeak# :: a -> b -> (State# RealWorld -> (# State# RealWorld, c #)) -> State# RealWorld -> (# State# RealWorld, Weak# b #)

- Removed from `GHC.Prim`:

        parAt# :: b -> a -> Int# -> Int# -> Int# -> Int# -> c -> Int#
        parAtAbs# :: a -> Int# -> Int# -> Int# -> Int# -> Int# -> b -> Int#
        parAtForNow# :: b -> a -> Int# -> Int# -> Int# -> Int# -> c -> Int#
        parAtRel# :: a -> Int# -> Int# -> Int# -> Int# -> Int# -> b -> Int#
        parGlobal# :: a -> Int# -> Int# -> Int# -> Int# -> b -> Int#
        parLocal# :: a -> Int# -> Int# -> Int# -> Int# -> b -> Int#

- Added to `GHC.Prim`:

        getSizeofMutableByteArray# :: MutableByteArray# d -> State# d -> (# State# d, Int# #)
        subWordC# :: Word# -> Word# -> (# Word#, Int# #)
        runRW# :: (State# RealWorld -> (# State# RealWorld, o #)) -> (# State# RealWorld, o #)

- Added to `GHC.Types`:

        data Module = Module TrName TrName
        data Nat
        data Symbol
        data TrName = TrNameS Addr# | TrNameD [Char]
        data TyCon = TyCon Word# Word# Module TrName
