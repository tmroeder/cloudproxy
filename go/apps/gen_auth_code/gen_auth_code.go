package main

import (
	"fmt"
	"go/ast"
	"go/parser"
	"go/token"
	"log"
)

func main() {
	// Hard-code the file for now.
	fset := token.NewFileSet()
	f, err := parser.ParseFile(fset, "../../tao/auth/ast.go", nil, 0)
	if err != nil {
		log.Fatal(err)
	}

	for _, d := range f.Decls {
		gd, ok := d.(*ast.GenDecl)
		if !ok || gd.Tok != token.TYPE {
			continue
		}

		for _, spec := range gd.Specs {
			// This should be an assertion, since it shouldn't be
			// possible for this to be anything but a type spec.
			ts, ok := spec.(*ast.TypeSpec)
			if !ok {
				continue
			}

			fmt.Printf("Considering type %s\n", ts.Name.Name)

			switch ts.Type.(type) {
			case *ast.ArrayType:
				fmt.Printf("It's an array\n")
			case *ast.Ident:
				fmt.Printf("It's an identifier\n")
			case *ast.InterfaceType:
				fmt.Printf("It's an interface\n")
			case *ast.StructType:
				fmt.Printf("It's a struct\n")
			default:
				fmt.Printf("It's neither\n")
			}
		}
	}

	fmt.Println("Done parsing the file")
}

// IAH: Each type should create a class except the Ident cases, which should be
// written out in a separate utility class, since they can be hard coded.
//
// Each type inherits from the interface that it implements (this will require
// something like reflection to implement cleanly, or I could hard-code it. Ick)
