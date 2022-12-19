{-# LANGUAGE CPP #-}
{-# LANGUAGE NoRebindableSyntax #-}
{-# OPTIONS_GHC -fno-warn-missing-import-lists #-}
module Paths_template_haskell (
    version,
    getBinDir, getLibDir, getDynLibDir, getDataDir, getLibexecDir,
    getDataFileName, getSysconfDir
  ) where

import qualified Control.Exception as Exception
import Data.Version (Version(..))
import System.Environment (getEnv)
import Prelude

#if defined(VERSION_base)

#if MIN_VERSION_base(4,0,0)
catchIO :: IO a -> (Exception.IOException -> IO a) -> IO a
#else
catchIO :: IO a -> (Exception.Exception -> IO a) -> IO a
#endif

#else
catchIO :: IO a -> (Exception.IOException -> IO a) -> IO a
#endif
catchIO = Exception.catch

version :: Version
version = Version [2,15,0,0] []
bindir, libdir, dynlibdir, datadir, libexecdir, sysconfdir :: FilePath

bindir     = "/usr/local/bin"
libdir     = "/usr/local/lib/x86_64-linux-ghc-8.6.5/template-haskell-2.15.0.0"
dynlibdir  = "/usr/local/lib/x86_64-linux-ghc-8.6.5"
datadir    = "/usr/local/share/x86_64-linux-ghc-8.6.5/template-haskell-2.15.0.0"
libexecdir = "/usr/local/libexec/x86_64-linux-ghc-8.6.5/template-haskell-2.15.0.0"
sysconfdir = "/usr/local/etc"

getBinDir, getLibDir, getDynLibDir, getDataDir, getLibexecDir, getSysconfDir :: IO FilePath
getBinDir = catchIO (getEnv "template_haskell_bindir") (\_ -> return bindir)
getLibDir = catchIO (getEnv "template_haskell_libdir") (\_ -> return libdir)
getDynLibDir = catchIO (getEnv "template_haskell_dynlibdir") (\_ -> return dynlibdir)
getDataDir = catchIO (getEnv "template_haskell_datadir") (\_ -> return datadir)
getLibexecDir = catchIO (getEnv "template_haskell_libexecdir") (\_ -> return libexecdir)
getSysconfDir = catchIO (getEnv "template_haskell_sysconfdir") (\_ -> return sysconfdir)

getDataFileName :: FilePath -> IO FilePath
getDataFileName name = do
  dir <- getDataDir
  return (dir ++ "/" ++ name)
