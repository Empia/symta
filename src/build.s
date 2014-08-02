use prelude compiler reader macro

GRootFolder = Void
GSrcFolders = Void
GDstFolder = Void
GHeaderTimestamp = Void
GMacros = Void

read_normalized Text =
| Expr = read Text
| case Expr [`|` @As] Expr
            X [`|` X]

skip_macros Xs = Xs.skip{X => X.1.is_macro}

// FIXME: do caching
get_lib_exports LibName =
| for Folder GSrcFolders
  | LibFile = "[Folder][LibName].s"
  | when file_exists LibFile
    | Text = load_text LibFile
    | Expr = read_normalized Text
    | leave: case Expr.last [export @Xs] | skip_macros Xs
                            Else | Void
| bad "no [LibName].s"

c_runtime_compiler Dst Src =
| RtFolder = "[GRootFolder]runtime"
| unix "gcc -O1 -Wno-return-type -Wno-pointer-sign -I '[RtFolder]' -g -o '[Dst]' '[Src]'"

c_compiler Dst Src =
| RtFolder = "[GRootFolder]runtime"
| unix "gcc -O1 -Wno-return-type -Wno-pointer-sign -I '[RtFolder]' -g -fpic -shared -o '[Dst]' '[Src]'"

file_older Src Dst =
| DstDate = if file_exists Dst then file_time Dst else 0
| Src^file_time << DstDate and GHeaderTimestamp << DstDate

compile_runtime Src Dst =
| when file_older Src Dst: leave Void
| say "compiling runtime..."
| Result = c_runtime_compiler Dst Src
| when Result <> "": bad Result

add_imports Expr Deps =
| unless Deps.size > 0: leave Expr
| [[_fn (map D Deps D.1) Expr]
   @(map D Deps [_import [_quote D.0] [_quote D.1]])]

compile_expr Name Dst Expr =
| Uses = [core]
| Expr <= case Expr
            [`|` [use @Us] @Xs]
               | Uses <= [@Uses @Us]
               | [`|` @Xs]
            Else | Expr
| Deps = Uses.tail
| for D Deps: unless compile_module D: bad "cant compile [D].s"
| say "compiling [Name]..."
| Imports = (map U Uses: map E U^get_lib_exports: [U E]).join
| ExprWithDeps = add_imports Expr Imports
| ExpandedExpr = macroexpand ExprWithDeps GMacros
| CFile = "[Dst].c"
| ssa_produce_file CFile ExpandedExpr
| Result = c_compiler Dst CFile
| unless file_exists Dst: bad "[CFile]: [Result]"
| Deps

load_symta_file Filename =
| Text = load_text Filename
| read_normalized Text

compile_module Name =
| for Folder GSrcFolders
  | SrcFile = "[Folder][Name].s"
  | when file_exists SrcFile
    | DstFile = "[GDstFolder][Name]"
    | DepFile = "[DstFile].dep"
    | when file_exists DepFile
      | Deps = DepFile^load_symta_file.1
      | CompiledDeps = map D Deps: compile_module D
      | when file_older SrcFile DstFile and CompiledDeps.all{X => have X and file_older X DstFile}:
        | leave DstFile
    | Expr = load_symta_file SrcFile
    | Deps = compile_expr Name DstFile Expr
    | DepsText = Deps.infix{' '}.unchars
    | save_text DepFile DepsText
    | leave DstFile
| Void

build BuildFolder =
| MacrosLib = '/Users/nikita/Documents/git/symta/build/lib/macro'
| let GMacros MacrosLib^load_library.keep{X => X.1.is_macro}.as_table
      GRootFolder "/Users/nikita/Documents/git/symta/"
      GSrcFolders ["[BuildFolder]src/" "[GRootFolder]src/"]
      GDstFolder "[BuildFolder]lib/"
      GHeaderTimestamp (file_time "[GRootFolder]/runtime/runtime.c")
  | RuntimeSrc = "[GRootFolder]/runtime/runtime.c"
  | RuntimePath = "[BuildFolder]run"
  | compile_runtime RuntimeSrc RuntimePath
  | DstFile = compile_module main
  | when no DstFile: bad "cant compile main.s"
  | unix "[RuntimePath] '[GDstFolder]'"


export build
