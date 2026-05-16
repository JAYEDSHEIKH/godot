# NEX Language — Godot Edition
> Statically typed. Immutable by default. Faster than GDScript.  
> Built for Godot. Designed like C++. Written like Rust.

---

## WHY NEX IS FASTER THAN GDSCRIPT

GDScript is a dynamic language. Every value at runtime is a `Variant` — a fat 20-byte
union that carries a type tag. Every single operation checks that tag:

```
GDScript runtime cost for:  a + b
  1. Load Variant a from stack
  2. Read type tag of a       ← extra work
  3. Load Variant b from stack
  4. Read type tag of b       ← extra work
  5. Call Variant::operator+  ← dispatches based on tags
  6. Allocate result Variant  ← memory allocation
  7. Store result to stack
```

NEX knows types at compile time. The same addition:

```
NEX runtime cost for:  a + b  (where a, b: int)
  1. Load i64 from slot[a]
  2. ADD i64 slot[b]
  3. Store i64 to slot[result]
```

No tag checks. No dispatch. No allocation. Pure CPU arithmetic.

This is the core difference. Every feature of NEX is designed to make this possible.

---

## PERFORMANCE DESIGN PRINCIPLES

| NEX Feature | Why It's Faster |
|---|---|
| Immutable by default (`x := 5`) | Compiler treats as constant — inlined, never reloaded |
| `var` required for mutability | Enables register allocation for all immutable bindings |
| Explicit types (`x: int`) | No runtime dispatch — direct typed CPU instructions |
| `struct` instead of classes | Flat memory — no vtable, no refcount overhead, no hashmap |
| `impl` methods | Direct function pointer calls — no virtual dispatch |
| `Option<T>` / `Result<T,E>` | No null, no exceptions — tag check + conditional jump only |
| `match` exhaustive | Compiler-verified — integer match compiles to jump table |
| `const` | Folded at compile time — zero runtime cost, doesn't exist in output |
| No implicit conversions | No hidden boxing or unboxing between numeric types |
| `@inline` annotation | Function body substituted at call site — call overhead is zero |

---

## CHAPTER 1 — Your First Program

```nex
print("hello world");
```

No imports. No main. No boilerplate.

**Godot script:**
```nex
extends Node2D;

fn _ready {
    print("hello from NEX!");
}
```

Every Godot NEX script starts with `extends`. Lifecycle functions replace `fn main`.

---

## CHAPTER 2 — Variables

### Immutable (default) — compiler-level constant
```nex
name := "ali";
age  := 20;
pi   := 3.14159;
flag := true;
```

Immutable bindings are treated as constants in the IR. After declaration the compiler
substitutes the value directly at every use site — no memory load on subsequent reads
if the value fits in a register or is small enough to inline.

### Mutable — typed stack slot
```nex
var score := 0;
var lives := 3;

score = 100;
lives -= 1;
```

`var` allocates a **typed** slot. The slot type is sealed at declaration. You cannot
assign a different type later — caught at compile time, zero runtime cost.

### Explicit Types
```nex
x:     int   := 5;
ratio: float := 0.75;
label: str   := "test";
```

### Constants — zero runtime cost
```nex
const MAX_HEALTH := 100;
const GRAVITY    := 980.0;
const APP_NAME   := "MyGame";
```

`const` values are constant-folded entirely. Any expression using `MAX_HEALTH` is
replaced with `100` in the typed IR before code generation. The constant never
occupies memory at runtime.

### 🎮 Exported Variables (Godot Inspector)
```nex
@export speed:       float := 200.0;
@export health:      int   := 100;
@export player_name: str   := "Hero";

@export_range(0.0, 1000.0) speed: float := 200.0;
@export_enum("Walk", "Run", "Fly") move_mode: str := "Walk";
@export target: Node2D;

@export_group("Combat");
@export damage:       int   := 10;
@export attack_speed: float := 1.5;
@export_group_end;
```

### What the Compiler Rejects
```nex
x := 5;
x = 10;      // COMPILE ERROR — x is immutable

var y := 5;
y = "hello"; // COMPILE ERROR — y is int, not str
```

Both errors are caught before any bytecode is generated. Zero runtime cost.

---

## CHAPTER 3 — Basic Types

### Integers — raw machine integers
```nex
a := 100;               // int   → i64
b: int8   := 127;       // i8
c: int16  := 32000;     // i16
d: int32  := 2000000;   // i32
e: int64  := 9000000;   // i64
```

All integer arithmetic compiles to native CPU integer instructions. No Variant wrapping.
No tag check. No allocation.

### Floats — IEEE 754 direct
```nex
x := 3.14;              // float → f64
y: float32 := 3.14;     // f32
z: float64 := 3.14159;  // f64
```

### Boolean — single byte
```nex
is_active := true;    // bool → u8  (0 or 1)
is_done   := false;
```

### String — reference type
```nex
name := "ali";
full := "hello " + name;
msg  := "name={name}, age={age}";  // interpolation compiled once, not at runtime

bio := "
    line one
    line two
";
```

String interpolation is resolved at compile time into a sequence of concatenations.
It does not perform format parsing at runtime.

### Character — u32 codepoint
```nex
letter := 'a';
digit  := '5';
```

### 🎮 Godot Math Types — value types, no heap
```nex
pos:   Vector2    := Vector2(100.0, 200.0);   // 2x f32 — 8 bytes, stack slot
pos3:  Vector3    := Vector3(1.0, 0.0, 0.0);  // 3x f32 — 12 bytes
col:   Color      := Color(1.0, 0.0, 0.0, 1.0);// 4x f32 — 16 bytes
rect:  Rect2      := Rect2(0.0, 0.0, 100.0, 50.0);
xform: Transform2D := Transform2D.IDENTITY;
```

These map directly to Godot's math structs. In the typed VM they occupy fixed-size
slots — no heap allocation, no reference counting.

---

## CHAPTER 4 — Operators

### Arithmetic — one CPU instruction per operation
```nex
a := 10 + 5;    // i64 ADD
b := 10 - 5;    // i64 SUB
c := 10 * 5;    // i64 MUL
d := 10 / 5;    // f64 DIV (division always yields float)
e := 10 % 3;    // i64 MOD
f := 2 ** 8;    // CONSTANT FOLDED → 256 at compile time, not at runtime
```

`2 ** 8` with two literal integers is computed during compilation. It does not appear
in the output bytecode at all.

### Comparison — typed compare instructions
```nex
10 == 10;    // i64 EQ
10 != 5;     // i64 NEQ
10 > 5;      // i64 GT
10 >= 10;    // i64 GTE
```

### Logical — short-circuit with jumps
```nex
a and b;    // if a is false, b is never evaluated — conditional jump
a or b;     // if a is true, b is never evaluated
not a;      // bitwise NOT on bool slot
```

### Assignment Operators
```nex
var x := 10;
x += 5;     // in-place ADD to typed slot — no intermediate allocation
x -= 3;
x *= 2;
x /= 4;
x %= 4;
```

---

## CHAPTER 5 — Functions

### Basic Function
```nex
fn say_hello {
    print("hello!");
}

say_hello();
```

### Typed Parameters and Return — compiler-verified
```nex
fn add(a: int, b: int) -> int {
    ret a + b;    // i64 ADD — one instruction
}

result := add(3, 4);
```

Types verified at compile time. The `+` emits a direct `i64 ADD`. No Variant dispatch.

### Multiple Return Values — no heap allocation
```nex
fn minmax(a: int, b: int) -> (int, int) {
    if a < b { ret (a, b); }
    ret (b, a);
}

(min, max) := minmax(10, 3);
```

`(int, int)` is returned in two adjacent stack slots or two registers.
No tuple object allocated on the heap.

### Default Parameters
```nex
fn greet(name: str, greeting: str = "hello") {
    print("{greeting} {name}");
}

greet("ali");
greet("ali", "hey");
```

### Inline Functions — zero call overhead
```nex
@inline fn double(x: int) -> int { ret x * 2; }
@inline fn square(x: int) -> int { ret x * x; }
```

`@inline` replaces every call site with the function body. The call instruction
disappears entirely from the output. The compiler also auto-inlines functions
with 8 or fewer IR instructions even without the annotation.

### First-Class Functions — function pointers, not closures
```nex
fn apply(f: fn(int) -> int, x: int) -> int {
    ret f(x);    // direct function pointer call — no vtable
}

fn triple(x: int) -> int { ret x * 3; }

result := apply(triple, 5);    // 15
```

### 🎮 Godot Lifecycle
```nex
extends CharacterBody2D;

fn _ready { }                       // once, on scene entry
fn _process(dt: float) { }         // every frame — dt is f64 slot
fn _physics_process(dt: float) { } // every physics tick
fn _input(event: InputEvent) { }   // every input event
fn _exit_tree { }                   // on removal from tree
```

---

## CHAPTER 6 — Control Flow

### If / Elif / Else
```nex
if score >= 90 {
    print("A");
} elif score >= 80 {
    print("B");
} else {
    print("F");
}
```

Dead branches are eliminated when conditions are provably constant at compile time.

### Inline If — conditional move, no branch
```nex
status := "adult" if age >= 18 else "minor";
```

Compiles to a conditional move instruction where possible — zero branch overhead.

### For Loop — Range — pure integer loop
```nex
for i in 0..10   { }    // i: 0,1,2,...,9
for i in 0..=10  { }    // i: 0,1,2,...,10
for i in 10..0   { }    // descending
for i in 0..20 by 2 { } // i: 0,2,4,...,18
```

Compiles to a simple counter loop:
```
i = start
LOOP: if i >= end goto END
      ... body ...
      i += step
      goto LOOP
END:
```

No iterator object. No allocation. Identical to a C `for` loop.

### For — Collection
```nex
for name in names { print(name); }
for i, item in items { print("{i}: {item}"); }
```

Compiles to an indexed loop over the backing buffer. The iterator is a plain integer
counter. No iterator object created.

### While / Loop
```nex
var x := 0;
while x < 5 { x += 1; }

loop { if done { break; } }
```

### Match — jump table for integers
```nex
match code {
    200 -> print("ok"),
    404 -> print("not found"),
    500 -> print("server error"),
    _   -> print("other"),
}
```

Integer match compiles to a **jump table** — O(1) dispatch regardless of arm count.
Faster than a chain of if/elif regardless of number of cases.

### Match — Range Arms
```nex
match score {
    90..=100 -> print("A"),
    80..=89  -> print("B"),
    70..=79  -> print("C"),
    _        -> print("F"),
}
```

Range arms compile to ranged compare + jump pairs. No loop, no iteration.

---

## CHAPTER 7 — Arrays

```nex
numbers := [1, 2, 3, 4, 5];

first := numbers[0];     // direct pointer offset — one load
len   := numbers.len();  // reads length field — one load

var scores := [10, 20, 30];
scores.push(40);
scores.pop();
scores[0] = 99;

part    := numbers[1..3];
doubled := numbers.map(fn(x) { ret x * 2; });
big     := numbers.filter(fn(x) { ret x > 3; });
total   := numbers.reduce(0, fn(acc, x) { ret acc + x; });
has     := numbers.contains(3);
found   := numbers.find(fn(x) { ret x > 3; });
numbers.sort();
numbers.sort_by(fn(a, b) { ret a > b; });
```

### Typed Arrays — no per-element boxing
```nex
ints:   [int]   := [1, 2, 3];
floats: [float] := [1.0, 2.0, 3.0];
```

A `[int]` array stores raw `i64` values back-to-back in memory. No Variant wrapping
per element. Iteration reads consecutive memory — CPU cache-friendly.

---

## CHAPTER 8 — Maps

```nex
user := map[str, str] {
    "name": "ali",
    "city": "karachi",
};

name     := user["name"];
city     := user.get("city");    // Option<str>
user["email"] = "ali@example.com";
user.delete("city");
has_name := user.has("name");

for key, value in user { print("{key}: {value}"); }

count := user.len();
```

---

## CHAPTER 9 — Structs

Structs are **flat value types**. No vtable. No reference count. No property hashmap.
Field access is a direct memory offset computed at compile time.

```nex
struct Point {
    x: float,    // offset 0
    y: float,    // offset 4
}

p := Point { x: 3.0, y: 4.0 };
print(p.x);    // load f32 from offset 0 — one instruction
print(p.y);    // load f32 from offset 4 — one instruction
```

**GDScript property access:**
```
obj.x
→ hash property name
→ lookup in property HashMap
→ read Variant
→ unbox to float
= ~4 operations minimum
```

**NEX struct field access:**
```
p.x
→ load f32 from offset 0
= 1 operation
```

### Methods — direct function pointer calls
```nex
struct Rectangle {
    width: float,
    height: float,
}

impl Rectangle {
    fn area(self) -> float {
        ret self.width * self.height;    // 2 loads + 1 multiply
    }

    fn perimeter(self) -> float {
        ret 2.0 * (self.width + self.height);
    }

    fn is_square(self) -> bool {
        ret self.width == self.height;
    }

    fn new(w: float, h: float) -> Rectangle {
        ret Rectangle { width: w, height: h };
    }
}

rect := Rectangle.new(5.0, 3.0);
area := rect.area();    // direct call — no vtable, no hashmap, no dispatch
```

`rect.area()` compiles identically to a C function call `Rectangle_area(&rect)`.

### Mutable Self
```nex
impl Rectangle {
    fn scale(var self, factor: float) {
        self.width  *= factor;
        self.height *= factor;
    }
}
```

### Nested Structs — compile-time offset chain
```nex
struct Address { street: str, city: str }
struct User    { name: str, age: int, address: Address }

user := User {
    name: "ali", age: 20,
    address: Address { street: "main st", city: "karachi" },
};

print(user.address.city);    // compile-time offset chain — zero dynamic lookup
```

### 🎮 Struct as Godot Resource
```nex
@resource
struct SaveData {
    player_name: str,
    level:       int,
    score:       int,
    position:    Vector2,
}

data := SaveData { player_name: "ali", level: 5, score: 9000, position: Vector2(0,0) };
ResourceSaver.save(data, "user://save.tres");
loaded := ResourceLoader.load("user://save.tres") as SaveData;
```

---

## CHAPTER 10 — Enums

```nex
enum Color { Red, Green, Blue }

c := Color.Red;

match c {
    Color.Red   -> print("red"),
    Color.Green -> print("green"),
    Color.Blue  -> print("blue"),
}
```

Basic enums are integers: `Red == 0`, `Green == 1`. Match compiles to a jump table.
No string comparison. No hashmap. No Variant.

### Enum With Data — tagged union in memory
```nex
enum Message {
    Text(content: str),
    Image(url: str, width: int, height: int),
    Video(url: str, duration: int),
    Empty,
}

msg := Message.Text(content: "hello");

match msg {
    Message.Text(content)    -> print("text: {content}"),
    Message.Image(url, w, h) -> print("image {w}x{h}"),
    Message.Video(url, dur)  -> print("video {dur}s"),
    Message.Empty            -> print("empty"),
}
```

Memory layout: `[tag: u8] [payload: largest variant]`. The match reads the tag byte
and jumps. No dynamic allocation. No boxing.

### 🎮 Exported Enum
```nex
@export_enum
enum MoveState { Idle, Walking, Running, Jumping, Dead }

@export current_state: MoveState := MoveState.Idle;
```

---

## CHAPTER 11 — Option Type — no null, ever

```nex
fn find_user(id: int) -> Option<str> {
    if id == 1 { ret some("ali"); }
    ret none;
}

match find_user(99) {
    some(name) -> print("found: {name}"),
    none       -> print("not found"),
}

name := find_user(1).unwrap_or("unknown");

find_user(1).if_some(fn(name) { print("hello {name}"); });
```

`Option<int>` compiles to: `[present: bool] [value: i64]` — 9 bytes on the stack.
No heap. No nullable pointer. No null check at runtime — the compiler enforces it.

---

## CHAPTER 12 — Result Type — no exceptions

```nex
fn parse_int(s: str) -> Result<int, str> {
    if s == "" { ret err("empty string"); }
    ret ok(42);
}

match parse_int("42") {
    ok(num)  -> print("parsed: {num}"),
    err(msg) -> print("error: {msg}"),
}

fn read_and_parse(path: str) -> Result<int, str> {
    content := fs.read(path)?;    // ? = tag check + early return if err
    num     := parse_int(content)?;
    ret ok(num);
}
```

`?` compiles to: read tag, if tag == Err jump to return-err. Two instructions.
No stack unwinding. No exception table. No heap allocation for error propagation.

---

## CHAPTER 13 — Modules

```nex
// utils/math.nex
pub fn add(a: int, b: int) -> int { ret a + b; }
pub fn multiply(a: int, b: int) -> int { ret a * b; }
fn helper(x: int) -> int { ret x * 2; }    // private
```

```nex
// main.nex
use utils.math;
fn main {
    result := math.add(5, 3);    // resolved at compile time — direct call
}
```

### 🎮 Godot Autoloads
```nex
autoload GameManager  from "res://managers/game_manager.nex";
autoload AudioManager from "res://managers/audio_manager.nex";

GameManager.start_level(1);
AudioManager.play("music_main");
```

---

## CHAPTER 14 — Closures

```nex
double := fn(x: int) -> int { ret x * 2; };

multiplier := 3;
triple := fn(x: int) -> int { ret x * multiplier; };    // captures immutable

numbers := [1, 2, 3, 4, 5];
doubled := numbers.map(fn(x) { ret x * 2; });
evens   := numbers.filter(fn(x) { ret x % 2 == 0; });
```

Closures that capture only immutable values compile to plain function pointers —
zero overhead. No closure object allocated. Closures capturing mutable values
get a small capture struct on the stack only.

---

## CHAPTER 15 — Async / Await

```nex
async fn fetch(url: str) -> Result<str, str> {
    response := await net.get(url);
    match response {
        ok(r)  -> ret ok(r.body),
        err(e) -> ret err(e),
    }
}

async fn main {
    (users, posts) := await parallel(
        fetch("https://api.example.com/users"),
        fetch("https://api.example.com/posts"),
    );
}
```

### 🎮 Await Godot Signals
```nex
async fn _ready {
    await get_tree().create_timer(2.0).timeout;
    print("2 seconds passed");

    await player.died;
    show_game_over();
}
```

---

## CHAPTER 16 — File I/O

```nex
use fs;

match fs.read("user://data.txt") {
    ok(text) -> print(text),
    err(e)   -> print("error: {e}"),
}

fs.write("user://output.txt", "hello");
fs.append("user://log.txt", "entry\n");

for file in fs.list("./src") { print(file.name); }
```

### 🎮 Godot Paths
```nex
fs.read("res://data/config.json");    // project files, read-only in exports
fs.write("user://save.json", data);   // user directory, always writable
```

---

## CHAPTER 17 — Strings

```nex
s := "Hello World";
s.len(); s.upper(); s.lower(); s.trim();
s.contains("World"); s.starts_with("Hello");
s.replace("World", "NEX"); s.split(" ");
"-".join(["a", "b", "c"]);
s[0..5];
"ab".repeat(3);
"42".parse_int();    // ok(42)
```

---

## CHAPTER 18 — Math

```nex
use math;
math.abs(-5); math.sqrt(16.0); math.pow(2.0, 8.0);
math.floor(3.7); math.ceil(3.2); math.round(3.5);
math.min(3, 5); math.max(3, 5); math.clamp(x, 0, 100);
math.sin(a); math.cos(a); math.log(x);
math.random(); math.random_int(0, 10);
math.PI; math.E;
```

---

## CHAPTER 19 — 🎮 Signals

```nex
extends Node;

signal jumped;
signal health_changed(old_hp: int, new_hp: int);

fn take_damage(amount: int) {
    old := health;
    health -= amount;
    emit health_changed(old, health);
    if health <= 0 { emit jumped; }
}

fn _ready {
    player.health_changed.connect(fn(old, new) {
        print("HP: {old} -> {new}");
    });
    player.jumped.connect(on_player_jumped);
    door.opened.connect(on_door_opened, one_shot: true);
}
```

---

## CHAPTER 20 — 🎮 Node Access

```nex
extends Node2D;

fn _ready {
    sprite := $Sprite2D;
    label  := $"UI/Label";
    parent := get_parent();
    player := get_tree().get_first_node_in_group("player") as Player;

    match get_node_or_null("OptionalChild") {
        some(n) -> print("found: {n.name}"),
        none    -> print("not there"),
    }
}
```

---

## CHAPTER 21 — 🎮 Input

```nex
fn _input(event: InputEvent) {
    if event.is_action_pressed("jump")    { jump(); }
    if event.is_action_released("attack") { stop_attack(); }
}

fn _process(dt: float) {
    var dir := Vector2.ZERO;
    if Input.is_action_pressed("move_right") { dir.x += 1.0; }
    if Input.is_action_pressed("move_left")  { dir.x -= 1.0; }
    if Input.is_action_pressed("move_up")    { dir.y -= 1.0; }
    if Input.is_action_pressed("move_down")  { dir.y += 1.0; }

    velocity = dir.normalized() * speed;
    move_and_slide();
}
```

---

## CHAPTER 22 — 🎮 Complete Game Script

```nex
extends CharacterBody2D;

@export speed:      float := 300.0;
@export jump_force: float := -500.0;
@export max_health: int   := 100;

signal health_changed(new_health: int);
signal died;

var health:  int;
var is_dead: bool := false;

const GRAVITY := 980.0;

fn _ready {
    health = max_health;
}

fn _physics_process(dt: float) {
    if is_dead { ret; }

    if not is_on_floor() {
        velocity.y += GRAVITY * dt;
    }

    if Input.is_action_just_pressed("jump") and is_on_floor() {
        velocity.y = jump_force;
    }

    var dir := 0.0;
    if Input.is_action_pressed("move_right") { dir += 1.0; }
    if Input.is_action_pressed("move_left")  { dir -= 1.0; }
    velocity.x = dir * speed;

    move_and_slide();
    update_animation(dir);
}

fn take_damage(amount: int) {
    if is_dead { ret; }
    health = math.clamp(health - amount, 0, max_health);
    emit health_changed(health);
    if health <= 0 { die(); }
}

fn heal(amount: int) {
    if is_dead { ret; }
    health = math.clamp(health + amount, 0, max_health);
    emit health_changed(health);
}

async fn die {
    is_dead = true;
    emit died;
    $AnimationPlayer.play("death");
    await $AnimationPlayer.animation_finished;
    queue_free();
}

fn update_animation(dir: float) {
    anim := $AnimationPlayer;
    if not is_on_floor() {
        anim.play("jump");
    } elif dir != 0.0 {
        anim.play("walk");
        $Sprite2D.flip_h = dir < 0.0;
    } else {
        anim.play("idle");
    }
}
```

---

## Quick Reference

```
CORE NEX
  Variables     x := 5           immutable — compiler constant
                var x := 5       mutable — typed stack slot
                const X := 5     zero runtime cost — folded at compile time

  Types         int  int8  int16  int32  int64
                float  float32  float64
                bool  str  char
                [T]  map[K,V]  (T, U)
                Option<T>  Result<T, E>

  Functions     fn name(a: int) -> int { ret a; }
                @inline fn fast(x: int) -> int { ret x * 2; }

  Control       if / elif / else
                for i in 0..10        → pure C-style integer loop
                for i in 0..20 by 2   → stepped
                for item in arr       → indexed buffer scan
                while / loop / break / continue
                match x { val -> action, _ -> default }

  Structs       struct Pos { x: float, y: float }
                impl Pos { fn len(self) -> float { } }
                → flat memory layout, compile-time offsets, no vtable

  Enums         enum S { A, B }              → integer (0, 1, ...)
                enum M { Text(s: str), Empty } → tagged union

  Option        some(val) / none    → tag + value, no heap, no null
  Result        ok(val) / err(msg)  → tag + union, ? = 2 instructions

  Closures      fn(x: int) -> int { ret x * 2; }
                → plain function pointer if captures are immutable

GODOT ADDITIONS 🎮
  Extend        extends Node2D;
  Lifecycle     fn _ready { }
                fn _process(dt: float) { }
                fn _physics_process(dt: float) { }
                fn _input(event: InputEvent) { }
                fn _exit_tree { }

  Export        @export speed: float := 200.0;
                @export_range(0, 100) health: int := 100;

  Signals       signal hp_changed(new: int);
                emit hp_changed(health);
                node.hp_changed.connect(fn(n) { });
                await node.hp_changed;

  Nodes         $Sprite2D   →  get_node("Sprite2D")
                $"UI/Label" →  get_node("UI/Label")
  Paths         res://  /  user://
  Types         Vector2  Vector3  Color  Rect2  Transform2D

PERFORMANCE GUARANTEES
  int arithmetic    → i64 CPU instructions, no Variant tag checks
  float math        → f64 CPU instructions, no Variant dispatch
  struct access     → compile-time byte offset, one load instruction
  impl methods      → direct function pointer, no vtable, no hashmap
  const values      → zero runtime cost, eliminated at compile time
  immutable vars    → eligible for register allocation
  range loops       → integer counter, no iterator object, no allocation
  match integers    → jump table, O(1) regardless of arm count
  closures (pure)   → plain function pointer, zero overhead
  Option<int>       → bool + i64 on stack, no heap
  Result<T,E>       → tag + union on stack, no heap, no exceptions
  @inline fns       → zero call overhead, body substituted at call site
  string interp     → resolved at compile time, not at runtime
```

---

*NEX — typed like C++, safe like Rust, lives inside Godot.*