class C {
	a : Int;
	b : Bool;
	init(x : Int, y : Bool) : C {
           {
		a <- x;
		b <- y;
		self;
           }
	};
};

Class Main {
	main(): Object {
	 {
	  (new C).init(1,1);
	  (new C).init(1,true,3);
	  (new C).iinit(1,true);
	  1 + true;
	  if 1 then 2 else 3 fi;
	  let x : Int <- true in x;
	  let x : Int <- 0 in let x : Int <- 1 in x;
	  let a : Int <- 1, a : Int <- 2 in a;
	  x <- 1;
	  case "s" of i : Int => 1; esac;
	  (new C) = (new C);
	  not 1;
	  (new Object)@C.init(1, true);
	  no_such_name;
	 }
	};
};
