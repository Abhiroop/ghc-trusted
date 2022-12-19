{-# LINE 1 "libraries/ghc-heap/GHC/Exts/Heap/InfoTable.hsc" #-}
module GHC.Exts.Heap.InfoTable
    ( module GHC.Exts.Heap.InfoTable.Types
    , itblSize
    , peekItbl
    , pokeItbl
    ) where



import Prelude -- See note [Why do we import Prelude here?]
import GHC.Exts.Heap.InfoTable.Types

{-# LINE 16 "libraries/ghc-heap/GHC/Exts/Heap/InfoTable.hsc" #-}
import Foreign

-------------------------------------------------------------------------
-- Profiling specific code
--
-- The functions that follow all rely on PROFILING. They are duplicated in
-- ghc-heap/GHC/Exts/Heap/InfoTableProf.hsc where PROFILING is defined. This
-- allows hsc2hs to generate values for both profiling and non-profiling builds.

-- | Read an InfoTable from the heap into a haskell type.
-- WARNING: This code assumes it is passed a pointer to a "standard" info
-- table. If tables_next_to_code is enabled, it will look 1 byte before the
-- start for the entry field.
peekItbl :: Ptr StgInfoTable -> IO StgInfoTable
peekItbl a0 = do

{-# LINE 35 "libraries/ghc-heap/GHC/Exts/Heap/InfoTable.hsc" #-}
  let ptr = a0
      entry' = Nothing

{-# LINE 38 "libraries/ghc-heap/GHC/Exts/Heap/InfoTable.hsc" #-}
  ptrs'   <- ((\hsc_ptr -> peekByteOff hsc_ptr 0)) ptr
{-# LINE 39 "libraries/ghc-heap/GHC/Exts/Heap/InfoTable.hsc" #-}
  nptrs'  <- ((\hsc_ptr -> peekByteOff hsc_ptr 4)) ptr
{-# LINE 40 "libraries/ghc-heap/GHC/Exts/Heap/InfoTable.hsc" #-}
  tipe'   <- ((\hsc_ptr -> peekByteOff hsc_ptr 8)) ptr
{-# LINE 41 "libraries/ghc-heap/GHC/Exts/Heap/InfoTable.hsc" #-}

{-# LINE 42 "libraries/ghc-heap/GHC/Exts/Heap/InfoTable.hsc" #-}
  srtlen' <- ((\hsc_ptr -> peekByteOff hsc_ptr 12)) a0
{-# LINE 43 "libraries/ghc-heap/GHC/Exts/Heap/InfoTable.hsc" #-}

{-# LINE 46 "libraries/ghc-heap/GHC/Exts/Heap/InfoTable.hsc" #-}
  return StgInfoTable
    { entry  = entry'
    , ptrs   = ptrs'
    , nptrs  = nptrs'
    , tipe   = toEnum (fromIntegral (tipe' :: HalfWord))
    , srtlen = srtlen'
    , code   = Nothing
    }

pokeItbl :: Ptr StgInfoTable -> StgInfoTable -> IO ()
pokeItbl a0 itbl = do

{-# LINE 60 "libraries/ghc-heap/GHC/Exts/Heap/InfoTable.hsc" #-}
  ((\hsc_ptr -> pokeByteOff hsc_ptr 0)) a0 (ptrs itbl)
{-# LINE 61 "libraries/ghc-heap/GHC/Exts/Heap/InfoTable.hsc" #-}
  ((\hsc_ptr -> pokeByteOff hsc_ptr 4)) a0 (nptrs itbl)
{-# LINE 62 "libraries/ghc-heap/GHC/Exts/Heap/InfoTable.hsc" #-}
  ((\hsc_ptr -> pokeByteOff hsc_ptr 8)) a0 (toHalfWord (fromEnum (tipe itbl)))
{-# LINE 63 "libraries/ghc-heap/GHC/Exts/Heap/InfoTable.hsc" #-}

{-# LINE 64 "libraries/ghc-heap/GHC/Exts/Heap/InfoTable.hsc" #-}
  ((\hsc_ptr -> pokeByteOff hsc_ptr 12)) a0 (srtlen itbl)
{-# LINE 65 "libraries/ghc-heap/GHC/Exts/Heap/InfoTable.hsc" #-}

{-# LINE 68 "libraries/ghc-heap/GHC/Exts/Heap/InfoTable.hsc" #-}

{-# LINE 69 "libraries/ghc-heap/GHC/Exts/Heap/InfoTable.hsc" #-}
  let code_offset = a0 `plusPtr` ((16))
{-# LINE 70 "libraries/ghc-heap/GHC/Exts/Heap/InfoTable.hsc" #-}
  case code itbl of
    Nothing -> return ()
    Just (Left xs) -> pokeArray code_offset xs
    Just (Right xs) -> pokeArray code_offset xs

{-# LINE 75 "libraries/ghc-heap/GHC/Exts/Heap/InfoTable.hsc" #-}
  where
    toHalfWord :: Int -> HalfWord
    toHalfWord i = fromIntegral i

-- | Size in bytes of a standard InfoTable
itblSize :: Int
itblSize = ((16))
{-# LINE 82 "libraries/ghc-heap/GHC/Exts/Heap/InfoTable.hsc" #-}
