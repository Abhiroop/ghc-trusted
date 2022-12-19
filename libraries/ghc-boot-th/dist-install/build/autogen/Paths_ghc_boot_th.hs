{-# LANGUAGE CPP #-}
{-# LANGUAGE NoRebindableSyntax #-}
{-# OPTIONS_GHC -fno-warn-missing-import-lists #-}
module Paths_ghc_boot_th (
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
version = Version [8,8,2] []
bindir, libdir, dynlibdir, datadir, libexecdir, sysconfdir :: FilePath

bindir     = "/usr/local/bin"
libdir     = "/usr/local/lib/x86_64-linux-ghc-8.8.2/ghc-boot-th-8.8.2"
dynlibdir  = "/usr/local/lib/x86_64-linux-ghc-8.8.2"
datadir    = "/usr/local/share/x86_64-linux-ghc-8.8.2/ghc-boot-th-8.8.2"
libexecdir = "/usr/local/libexec/x86_64-linux-ghc-8.8.2/ghc-boot-th-8.8.2"
sysconfdir = "/usr/local/etc"

getBinDir, getLibDir, getDynLibDir, getDataDir, getLibexecDir, getSysconfDir :: IO FilePath
getBinDir = catchIO (getEnv "ghc_boot_th_bindir") (\_ -> return bindir)
getLibDir = catchIO (getEnv "ghc_boot_th_libdir") (\_ -> return libdir)
getDynLibDir = catchIO (getEnv "ghc_boot_th_dynlibdir") (\_ -> return dynlibdir)
getDataDir = catchIO (getEnv "ghc_boot_th_datadir") (\_ -> return datadir)
getLibexecDir = catchIO (getEnv "ghc_boot_th_libexecdir") (\_ -> return libexecdir)
getSysconfDir = catchIO (getEnv "ghc_boot_th_sysconfdir") (\_ -> return sysconfdir)

getDataFileName :: FilePath -> IO FilePath
getDataFileName name = do
  dir <- getDataDir
  return (dir ++ "/" ++ name)
