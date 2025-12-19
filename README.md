# PyRev
Pythonから利用可能なリバーシ(オセロ)のライブラリです。局面管理、合法手列挙と裏返る石の高速な計算に対応しています。このライブラリを用いることで、Python(CPython)のみでリバーシの局面データを実装するよりも遥かに高速なリバーシの思考エンジンを開発できます。

本ライブラリはAVX2に対応したCPUにおいて最も高速に動作しますが、非対応のCPUでも基本的に問題なく動作します。

## インストール
以下のコマンドを実行する前に、C/C++のコンパイラをインストールしてください。特にWindowsであれば、MSVCのcl.exeにパスを通しておいてください。

### pipを用いる場合
```
pip install git+https://github.com/Yoka346/PyRev
```

### uvを用いる場合
```
uv add git+https://github.com/Yoka346/PyRev
```

## 各種定数及び基本的な型

### BoardCoordinate型
盤面座標を表す整数値の型です。実体はnumpy.int8型であり、盤面座標は以下の順番で振られています。
盤面座標に限らず、多くの定数はできる限り小さいサイズの型で表現しています。思考エンジン開発においては、探索アルゴリズムを実装する際に、メモリ上に着手座標などの情報を大量に保持する場合(e.g. 置換表)があり、できる限り小さいデータ型を用いた方が有利だからです。

```
| 0| 1| 2| 3| 4| 5| 6| 7|
| 8| 9|10|11|12|13|14|15|
|16|17|18|19|20|21|22|23|
|24|25|26|27|28|29|30|31|
|32|33|34|35|36|37|38|39|
|40|41|42|43|44|45|46|47|
|48|49|50|51|52|53|54|55|
|56|57|58|59|60|61|62|63|

PASS_COORD = 64
NULL_COORD = 65
```

PASS_COORDはパスを表す特殊座標、NULL_COORDは無効な座標を表す特殊座標です。

また、各座標には、アルファベットと数字で表現される2次元座標が以下のように対応しています。

```
    A  B  C  D  E  F  G  H
1 | 0| 1| 2| 3| 4| 5| 6| 7|
2 | 8| 9|10|11|12|13|14|15|
3 |16|17|18|19|20|21|22|23|
4 |24|25|26|27|28|29|30|31|
5 |32|33|34|35|36|37|38|39|
6 |40|41|42|43|44|45|46|47|
7 |48|49|50|51|52|53|54|55|
8 |56|57|58|59|60|61|62|63|
```

これらの2次元座標をコード内でも利用できるように、全ての2次元座標は定数として定義されています。

```python
A1 = np.int8(0)
B1 = np.int8(1)
C1 = np.int8(2)
D1 = np.int8(3)
.
.
.
F8 = np.int8(61)
G8 = np.int8(62)
H8 = np.int8(63)
```

また、盤面座標は、parse_coord_str関数とcoord_to_str関数を用いて、文字列 <-> 盤面座標の相互変換を行うことができます。

```python
import pyrev

print(pyrev.F5)
print(pyrev.coord_to_str(pyrev.F5))
print(pyrev.parse_coord_str("F5"))
print(pyrev.parse_coord_str("f5"))


"""
result

37
F5
37
37
"""
```

### DiscColor型
Discとは、リバーシにおける石のことであり、その色を表現する型がDiscColor型です。実体はnumpy.int8型です。
それぞれの石の色は以下のように定義されています。

```python
BLACK: DiscColor = np.int8(0)
WHITE: DiscColor = np.int8(1)
NULL_COLOR: DiscColor = np.int8(2)
```

NULL_COLORは無効な色を表現する特殊値です。

DiscColorは、disc_color_to_str関数によって文字列に変換できます。
また、BLACK <-> WHITE間の変換は、to_opponent_color関数で行うことができます。ただし、to_opponent_color関数にNULL_COLORを与えた際の動作は未定義です。

```python
import pyrev

print(pyrev.disc_color_to_str(pyrev.BLACK))
print(pyrev.disc_color_to_str(pyrev.WHITE))

opp = pyrev.to_opponent_color(pyrev.BLACK)
print(pyrev.disc_color_to_str(opp))

opp = pyrev.to_opponent_color(pyrev.WHITE)
print(pyrev.disc_color_to_str(opp))

opp = pyrev.to_opponent_color(pyrev.NULL_COLOR) # undefined!

        
"""
result

Black
White
White
Black
"""
```

### SquareOwner型
Squareはマスを意味し、SquareOwnerはマスの所有者を表現する型です。SqureOwnerの実体はnumpy.int8型です。マスの所有者とは、そのマスに置かれている石の手番を意味し、現在の手番の石が置かれている場合はPLAYER、相手の石が置かれている場合はOPPONENTが所有者となります。空きマスの所有者はNULL_OWNERです。DiscColor以外にわざわざSquareOwnerを用意している理由は、石の色は大事ではないが、そのマスにある石が現在の手番の石なのか相手の石なのかは知りたいという状況が往々にしてあるためです。また、本ライブラリの実装上、あるマスにある石の色を確認するよりも、そのマスの所有者を確認する方が、条件分岐無しにチェックすることができ僅かに高速です。

```python
PLAYER: SquareOwner = np.int8(0)
OPPONENT: SquareOwner = np.int8(1)
NULL_OWNER: SquareOwner = np.int8(2)
```

## BoardCoordinateIteratorクラス
本ライブラリにおいて、合法手の座標リストや裏返る石の座標リストは、Pythonのlistではなく、64bit整数によって表現されます。例えば、64bit整数のLSBからみてm, n, k番目のビットが1であるとき、
その64bit整数で表現されるリストには、座標m, n, kが格納されていることを意味します。 
BoardCoordinateIteratorクラスは、このように64bit整数で表現された座標リストから、BoardCoordinate型の座標を列挙します。

```python
import pyrev
from pyrev import BoardCoordinateIterator
import numpy as np

bits = np.int64(0xf) # 下位4bitのみが1
for coord in BoardCoordinateIterator(bits):
    print(pyrev.coord_to_str(coord))


"""
result

A1
B1
C1
D1
"""
```

Pythonのlistに格納したい場合は、以下のようにlistコンストラクタにBoardCoordinateIteratorオブジェクトを渡します。

```python
coords = list(BoardCoordinateIterator(bits))
```

## Positionクラス
局面を表現するクラスです。

### side_to_moveプロパティ
現在の手番を表すDiscColor型の値を返します。

### empty_square_countプロパティ
空きマスの数を整数値で返します。

### player_disc_countプロパティ
現在の手番の石の数を整数値で返します。

### opponent_disc_countプロパティ
相手の石の数を整数値で返します。

### disc_countプロパティ
全ての石の数を整数値で返します。

### get_disc_count_of(DiscColor)メソッド
引数にDiscColor型の値を取り、その色の石の数を整数値で返します。
引数にNULL_COLORを与えた際の動作は未定義です。

### copy_to(Position)メソッド
引数で受け取ったPositionオブジェクトにオブジェクトの状態をコピーします。

### copy()メソッド
Positionオブジェクトのコピーを生成して返します。Positionのコピーを行う際は、既存のPositionオブジェクトに書き込む方がオブジェクトの生成を抑えられるため、実行速度がシビアな場面では、copy_toメソッドを利用することを推奨します。

### get_square_owner_at(BoardCoordinate)メソッド
引数で指定した座標のマスの所有者をSquareOwner型の値として返します。どのプレイヤーも所有していないマスの場合は、NULL_OWNERが返ります。

### get_square_color_at(BoardCoordinate)メソッド
引数で指定した座標のマスに置かれている石の色をSquareOwner型の値として返します。石が置かれていないマスの場合は、NULL_COLORが返ります。
石の色が重要ではない場面では、より軽量なget_square_color_atメソッドを利用することを推奨します。

### get_player_disc_coords()メソッド
現在の手番の石が配置されているマスの座標をBoardCoordinateIteratorオブジェクトで返します。

### get_opponent_disc_coords()メソッド
相手の石が配置されているマスの座標をBoardCoordinateIteratorオブジェクトで返します。

### get_empty_square_coords()メソッド
空きマスの座標をBoardCoordinateIteratorオブジェクトで返します。

### \_\_eq\_\_(Position)メソッド
Positionオブジェクトの等号演算子による比較を可能にします。Position.side_to_moveと石の配置が等しい場合にTrueが返ります。
Positionオブジェクト以外のオブジェクトと比較した場合にTypeErrorが発生します。

### \_\_str\_\_()メソッド
Positionオブジェクトを文字列で表現します。黒石は'*'、白石は'O'で表現されます。

```python
from pyrev import Position

pos = Position()
print(pos)


"""
result

  A B C D E F G H 
1 - - - - - - - - 
2 - - - - - - - - 
3 - - - - - - - - 
4 - - - O * - - - 
5 - - - * O - - - 
6 - - - - - - - - 
7 - - - - - - - - 
8 - - - - - - - - 
"""
```

### put_player_disc_at(BoardCoordinate)メソッド
引数で指定した座標のマスに現在の手番の石を配置します。**このメソッドでは、石を裏返す処置や手番を交代する処理は行われません。**
あくまでも、盤面に自由に石を配置したい場合に用います。

### put_opponent_disc_at(BoardCoordinate)メソッド
引数で指定した座標のマスに相手の石を配置します。**このメソッドでは、石を裏返す処置や手番を交代する処理は行われません。**
あくまでも、盤面に自由に石を配置したい場合に用います。

### remove_disc_at(BoardCoordinate)メソッド
引数で指定した座標のマスにある石を除去し、空きマスにします。

### is_gameover()メソッド
終局している場合にTrueを返します。終局の定義は、現在の手番と相手の手番の着手可能位置が0になった時です。

### get_score()メソッド
現在の手番からみた石差を整数値で返します。

### get_score_from(DiscColor)メソッド
引数で与えられた石の色の手番からみた石差を返します。

### can_pass()メソッド
現在の手番がパスできる場合にTrueを返します。パスができる条件は、現在の手番の着手可能位置が0かつ相手の着手可能位置が0より大きい時です。

### is_legal(BoardCoordinate)メソッド
現在の手番が引数で与えられた座標のマスにルール上着手可能である時、Trueを返します。

### do_pass()メソッド
手番を交代します。このメソッドは、Position.can_passの真偽に関わらず実行可能です。

### get_legal_moves()メソッド
現在の手番の合法手の座標をBoardCoordinateIteratorオブジェクトとして返します。

### calc_flip_discs(BoardCoordinate)メソッド
現在の手番が引数で与えられた座標のマスに着手した際に裏返る石の座標のリストを64bit整数(numpy.int64)で返します。

### do_move(BoardCoordinate, np.int64)
引数で与えられた座標に現在の手番の石を配置し、引数で与えられた裏返る石のリスト(64bit整数)に従って、現在の手番の石を相手の石に反転します。
このメソッドはcalc_flip_discsメソッド共に用いることを想定しています。また、引数にPASS_COORDを与えた際の動作は未定義です。パスを行う場合はPosition.do_passメソッドを利用してください。
引数にNULL_COORDを与えた際の動作は未定義です。

```python
import pyrev
from pyrev import Position

pos = Position()
flip = pos.calc_flip_discs(pyrev.F5)
pos.do_move(pyrev.F5, flip)
print(pos)


"""
result

 A B C D E F G H 
1 - - - - - - - - 
2 - - - - - - - - 
3 - - - - - - - - 
4 - - - O * - - - 
5 - - - * * * - - 
6 - - - - - - - - 
7 - - - - - - - - 
8 - - - - - - - - 
"""
```

### do_move_at(BoardCoordinate)メソッド
引数で与えられた座標に手番の石を配置し、さらに相手の石を裏返した後、Trueを返します。また、非合法手の座標が与えられた場合にはFalseを返します。このメソッドは、合法手判定を行うため、実行速度にシビアな場面では、Position.do_moveメソッドを利用することを推奨します。
引数にNULL_COORDを与えた際の動作は未定義です。

```python
import pyrev
from pyrev import Position

pos = Position()
print(pos.do_move_at(pyrev.F4)) # 非合法手
print(pos)

print()

print(pos.do_move_at(pyrev.F5)) # 合法手
print(pos)


"""
result

False
  A B C D E F G H 
1 - - - - - - - - 
2 - - - - - - - - 
3 - - - - - - - - 
4 - - - O * - - - 
5 - - - * O - - - 
6 - - - - - - - - 
7 - - - - - - - - 
8 - - - - - - - - 

True
  A B C D E F G H 
1 - - - - - - - - 
2 - - - - - - - - 
3 - - - - - - - - 
4 - - - O * - - - 
5 - - - * * * - - 
6 - - - - - - - - 
7 - - - - - - - - 
8 - - - - - - - - 
"""
```

### calc_hash_codeメソッド
64bitの局面のハッシュ値を返します。ハッシュ値はZobristのアルゴリズムで算出します。置換表などで利用することを想定しています。

## 謝辞
合法手列挙や裏返る石の位置算出には、以下のサイトを大いに参考にしています。

http://www.amy.hi-ho.ne.jp/okuhara/bitboard.htm

また、CythonとC++を用いてライブラリを開発するにあたり、以下のリポジトリも参考にしています。

https://github.com/TadaoYamaoka/creversi

その他、参考にした文献です。

https://github.com/gikou-official/Gikou  
https://github.com/HiraokaTakuya/apery

