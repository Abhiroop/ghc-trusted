{-# LANGUAGE CPP #-}
{-# LANGUAGE NoRebindableSyntax #-}
{-# OPTIONS_GHC -fno-warn-missing-import-lists #-}
module Paths_ghc_pkg (
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
version = Version [6,9] []
bindir, libdir, dynlibdir, datadir, libexecdir, sysconfdir :: FilePath

bindir     = "/usr/local/bin"
libdir     = "/usr/local/lib/x86_64-linux-ghc-8.6.5/ghc-pkg-6.9-ghc-pkg"
dynlibdir  = "/usr/local/lib/x86_64-linux-ghc-8.6.5"
datadir    = "/usr/local/share/x86_64-linux-ghc-8.6.5/ghc-pkg-6.9"
libexecdir = "/usr/local/libexec/x86_64-linux-ghc-8.6.5/ghc-pkg-6.9"
sysconfdir = "/usr/local/etc"

getBinDir, getLibDir, getDynLibDir, getDataDir, getLibexecDir, getSysconfDir :: IO FilePath
getBinDir = catchIO (getEnv "ghc_pkg_bindir") (\_ -> return bindir)
getLibDir = catchIO (getEnv "ghc_pkg_libdir") (\_ -> return libdir)
getDynLibDir = catchIO (getEnv "ghc_pkg_dynlibdir") (\_ -> return dynlibdir)
getDataDir = catchIO (getEnv "ghc_pkg_datadir") (\_ -> return datadir)
getLibexecDir = catchIO (getEnv "ghc_pkg_libexecdir") (\_ -> return libexecdir)
getSysconfDir = catchIO (getEnv "ghc_pkg_sysconfdir") (\_ -> return sysconfdir)

getDataFileName :: FilePath -> IO FilePath
getDataFileName name = do
  dir <- getDataDir
  return (dir ++ "/" ++ name)
