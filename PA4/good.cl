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

class D inherits C {
        is_ok() : Bool { b };
};

Class Main {
	main(): Object {
	 {
	  if (new D).is_ok() then
	    case (new Object) of
	      d : D => d;
	      c : C => c;
	      o : Object => o;
	    esac
	  else
	    new C
	  fi;
	  new IO.out_string("ok").out_int(42);
	  isvoid self;
	  (new C)@C.init(1, true);
	  1 + 2;
	  ~3;
	  4 = 4;
	  5 <= 6;
	  not false;
	 }
	};
};
