class Node {
  item: String;
  nil: Node;
  next: Node;
  pre: Node;
  init(x: String): Node {
    {
      item <- x;
      self;
    }
  };
  upd_item (x: String): Object {
    item <- x
  };
  get_item (): String {
    item
  };
  upd_next (x: Node): Object {
    next <- x
  };
  get_next (): Node {
    next
  };
  upd_pre (x: Node): Object {
    pre <- x
  };
  get_pre (): Node {
    pre
  };
};

class Stack inherits A2I{
  bot: Node <- (new Node).init("");
  top: Node <- bot;
  get_bot (): Node {
    bot
  };
  get_top (): Node {
    top
  };
  push (x: Node): Object {
    {
      top.upd_next(x);
      x.upd_pre(top);
      top <- x;
    }
  };
  pop (): Node {
    if top = bot then bot else
      let
        temp: Node <- top
      in
        { 
          top <- top.get_pre();
          temp;
        }
    fi
  };
  eval (): Object {
    let
      fun: Node <- pop(),
      a: Node <- pop(),
      b: Node <- pop(),
      temp: Node
    in
      {
        if fun.get_item() = "+" then
          {
            temp <- (new Node).init(i2a(a2i(a.get_item()) + a2i( b.get_item())));
            push(temp);
          }
        else {
          push(a);
          push(b);
        }
        fi;
      }
  };
};

class Main inherits IO {
  main(): Object {
    let 
      con: Bool <- true,
      inp: String,
      st: Stack <- new Stack,
      temp: Node
    in
      while con loop {
        out_string(">");
	inp <- in_string();
	if inp = "x" then con <- false  else {
	  if inp = "d" then {
	    temp <- st.get_top();
	    if temp = st.get_bot() then con <- false else con <- true fi;
	    while con loop {
	      out_string(temp.get_item().concat("\n"));
	      temp <- temp.get_pre();
	      if temp = st.get_bot() then con <- false else con <- con fi;
	    }
	    pool;
	    con <- true;
	  }
	  else {
            if inp = "e" then st.eval() else st.push((new Node).init(inp)) fi;
          }
	  fi;
	}
	fi;
      }
      pool
  };
};
