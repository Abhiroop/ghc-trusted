{-# LINE 1 "libraries/ghc-heap/GHC/Exts/Heap/Constants.hsc" #-}
{-# LANGUAGE CPP #-}

module GHC.Exts.Heap.Constants
    ( wORD_SIZE
    , tAG_MASK
    , wORD_SIZE_IN_BITS
    ) where



import Prelude -- See note [Why do we import Prelude here?]
import Data.Bits

wORD_SIZE, tAG_MASK, wORD_SIZE_IN_BITS :: Int
wORD_SIZE = 8
{-# LINE 16 "libraries/ghc-heap/GHC/Exts/Heap/Constants.hsc" #-}
wORD_SIZE_IN_BITS = 64
{-# LINE 17 "libraries/ghc-heap/GHC/Exts/Heap/Constants.hsc" #-}
tAG_MASK = (1 `shift` 3) - 1
{-# LINE 18 "libraries/ghc-heap/GHC/Exts/Heap/Constants.hsc" #-}
