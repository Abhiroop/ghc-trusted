{-# LINE 1 "libraries/ghci/GHCi/InfoTable.hsc" #-}
{-# LANGUAGE CPP, MagicHash, ScopedTypeVariables #-}

-- Get definitions for the structs, constants & config etc.


-- |
-- Run-time info table support.  This module provides support for
-- creating and reading info tables /in the running program/.
-- We use the RTS data structures directly via hsc2hs.
--
module GHCi.InfoTable
  (

{-# LINE 16 "libraries/ghci/GHCi/InfoTable.hsc" #-}
  ) where

import Prelude -- See note [Why do we import Prelude here?]

{-# LINE 26 "libraries/ghci/GHCi/InfoTable.hsc" #-}

ghciTablesNextToCode :: Bool

{-# LINE 29 "libraries/ghci/GHCi/InfoTable.hsc" #-}
ghciTablesNextToCode = True

{-# LINE 33 "libraries/ghci/GHCi/InfoTable.hsc" #-}


{-# LINE 386 "libraries/ghci/GHCi/InfoTable.hsc" #-}
