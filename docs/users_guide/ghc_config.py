extlinks = {
    'ghc-ticket': ('https://gitlab.haskell.org/ghc/ghc/issues/%s', 'Issue #'),
    'ghc-wiki': ('https://gitlab.haskell.org/ghc/ghc/wikis/%s', 'GHC Wiki'),
}

libs_base_uri = '../libraries'

# N.B. If you add a package to this list be sure to also add a corresponding
# LIBRARY_VERSION macro call to configure.ac.
lib_versions = {
    'base': '4.13.0.0',
    'ghc-prim': '0.5.3',
    'template-haskell': '2.15.0.0',
    'ghc-compact': '0.1.0.0',
    'ghc': '8.8.2',
    'parallel': '',
    'Cabal': '3.0.1.0',
    'array': '0.5.4.0',
}

version = '8.8.2'

llvm_version = '7'
