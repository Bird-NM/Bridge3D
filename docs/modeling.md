# Mô hình hình học và phép biến đổi

## Dữ liệu và kiến trúc

`model.hpp/cpp` chứa Parameters, Mesh, Component và Model. Mỗi cấu kiện có ID, tên, vật liệu, màu, cờ hiển thị, vai trò và hộp bao. Vertex gồm vị trí, pháp tuyến và trọng số dịch chuyển tại đầu dây. Mesh chứa riêng chỉ số tam giác và chỉ số cạnh có ý nghĩa. `renderer` quản lý VAO/VBO/EBO, shader và chế độ vẽ. `camera` xử lý orbit và phép chiếu. `main` quản lý ứng dụng, UI, picking, tải và ảnh PNG.

Hàm box tạo sáu mặt có pháp tuyến riêng để giữ cạnh sắc. Hàm beam quét tiết diện đa giác trên đường tâm, sinh hai vòng đỉnh, tam giác thành bên và nắp. Các cạnh vòng và cạnh dọc được lưu riêng. Chế độ Triangle edges sinh cả ba cạnh mỗi tam giác. Các mesh tĩnh tải lên GPU khi đổi cầu, sử dụng lại qua các khung hình. Tài nguyên GPU được giải phóng trước khi hủy context.

## Cầu treo

Trục X theo chiều dài, Y thẳng đứng, Z theo bề rộng. Hai tháp đặt tại ±towerX, hai tuyến cáp tại Z=±width/2. Nhịp giữa dùng parabol `y=T-sag*(1-x*x/towerX²)`. Nhịp bên dùng đoạn thẳng về mố. Đây là giả định hình học. Danh sách nút cáp bao gồm nút chia đều, vị trí dây treo và vị trí tháp để các điểm nối dùng cùng tọa độ. Dây treo đứng nối xuống cao độ dầm.

Hai bên giàn có thanh dưới, chéo chéo và liên kết ngang/chéo theo bề rộng. Mặt cầu chia theo panel để có đủ đỉnh quan sát biến dạng. Tháp cổng gồm hai chân, thanh ngang, bệ và mố neo thể tích.

## Cầu Hiếu

Một tháp tại X=`hieuTowerX` có hai chân ở Z âm/dương. Các tiết diện chữ nhật của chân giữ X chung và thu dần theo Z ở vùng cao. Vùng từ mặt cầu đến hết chiều cao thông xe cộng khoảng đệm giữ chân ở ngoài cả lề đi bộ. Hàm `hieuLegCenter` là nguồn tọa độ dùng chung cho chân, chi tiết chevron và neo dây. Các chi tiết chevron màu đất nung là thanh thể tích. Hai mặt phẳng dây thẳng kéo từ nút trên đường tâm chân tháp đến mép dầm. Không có mesh cáp cong trong mô hình này. Dầm hộp, hai dầm mép dọc đỡ neo dây, lan can, bệ trung tâm và trụ dẫn dùng mesh thể tích. Dầm ngang liên kết hai chân nằm dưới mặt cầu. Các mảng chevron nằm trên vùng thông xe. Mỗi dây có cặp điểm `CableAttachment` để kiểm tra vị trí neo vào tháp và dầm.

`carriagewayWidth`, `sidewalkWidth`, `sidewalkRaise`, `towerLegWidth`, `towerLegDepth`, `lateralClearance`, `vehicleClearanceHeight` và `hieuTowerX` nằm trong `config/bridges.cfg`. Tổng bề rộng bằng phần xe chạy cộng hai lề. Tại vùng thông xe, tâm chân cách tim đường `width/2 + lateralClearance + towerLegWidth/2`. Khi lên cao, smoothstep đưa chân dần vào đỉnh. Toàn bộ các giá trị này là giả định phục vụ mô hình.

Golden Gate có thanh biên trên liên tục tại đường tâm dây treo, thanh biên dưới, chéo bên và các liên kết ngang. Khối neo nằm ngoài mép đường. Mặt đường chia panel nhưng bỏ các mặt đứng nội bộ để tránh vệt ghép giả trên mặt nhựa. Lề được nâng nhẹ và vạch giữa đường nằm sát mặt nhựa. Cáp chính dùng tiết diện 16 cạnh, dây nhỏ dùng 12 cạnh.

Toàn bộ cấu hình hiện tại là giả định từ ảnh. Không có thông số được xác nhận bằng bản vẽ thi công. Không suy diễn chúng thành kích thước công trình thực.

## Camera, ánh sáng và lựa chọn

Orbit dùng yaw/pitch/distance và điểm mục tiêu. Fit sử dụng bán kính cầu bao, FOV và tỷ lệ viewport. Hai viewport so sánh có cùng camera và từng phép chiếu riêng. Near phụ thuộc khoảng cách, far bao cả cảnh. Nội suy mũ theo delta time cho phép chuyển động mượt và có thể tắt. Chuột chỉ tác động trong vùng xem khi ImGui không giữ đầu vào. Kích thước framebuffer được quy đổi từ tọa độ cửa sổ cho DPI.

Picking dùng ray từ nghịch đảo projection-view và giao tam giác Möller–Trumbore, xét cả hai phía. Chỉ mesh hiện nhận chọn. Vị trí đỉnh CPU dùng cùng công thức biến dạng và explode với shader. Kéo vượt 3 pixel chuyển thành orbit, thả nhấp mới chọn. Chọn tô màu cả nhóm và cho phép camera tập trung vào hộp bao.

Mặt vật thể dùng ánh sáng khuếch tán hai phía và thành phần ambient. Pháp tuyến dùng xấp xỉ nghịch đảo chuyển vị Jacobian của trường biến dạng để chiếu sáng trực quan. Chế độ cạnh dùng cùng đỉnh và ma trận với mặt. Polygon offset và prepass toàn cảnh hỗ trợ ẩn cạnh khuất. Mặt kết hợp cạnh dùng depth bias để giảm z-fighting.

## Minh họa biến dạng

Mỗi nhịp được giới hạn bởi hai gối. Golden Gate có gối tại hai đầu và hai tháp, Hiếu tại hai đầu và X=`hieuTowerX`. Với `t=(x-left)/(right-left)`, đặt

```
d(x) = -A * sin(pi*t)^2 * exp(-((x-loadX)/45)^2)
A = (relativeLoad/100) * magnification * 0.25
w(y) = clamp((towerHeight-y)/(towerHeight-deckHeight-2), 0, 1)
y' = y + d(x)*w(y)
```

Tháp và móng giữ nguyên. Các đỉnh tại đầu neo dây văng trên tháp có trọng số mobility=0, các đỉnh tại neo dầm có mobility=1. Vì vậy neo trên tháp đứng yên và dây văng vẫn là đoạn thẳng khi neo dầm dịch chuyển. Các nhóm dầm, đường, lan can và dây đều dùng cùng trường dịch chuyển. Gối có d(x)=0, vùng dầm có w=1, dây trên cao chuyển tiếp về 0. Các đầu dây cùng tọa độ neo chịu cùng phép biến đổi. Mặt cắt dầm chuyển nhất quán. Đây là trường động học quy ước, không được suy ra từ độ cứng vật liệu, tải trọng có đơn vị hay phương trình cân bằng. Tải tương đối là tham số minh họa. Xe chạy theo delta time. Chế độ explode tách các nhóm để quan sát, độc lập với giả định biến dạng.

Hình ban đầu có thể chồng bằng cạnh màu mờ. Không tạo bản đồ ứng suất hoặc kết quả kiểm định.


## Kiểm tra vùng thông xe

Bài kiểm tra dùng SAT giữa tam giác và hộp 3D, xét cả mặt và cạnh tam giác. Nó kiểm tra toàn chiều dài phần xe chạy với khoảng hở ngang và toàn chiều cao thông xe, hai lề đến chiều cao người đi bộ, và hộp bao xe thật tại 61 vị trí đi qua tháp. Thử lại với phần xe rộng 10, lề rộng 2, thông xe cao 7 và tháp dịch đến X=12 để bắt lỗi gắn cứng tọa độ.

Kiểm tra điểm neo gồm số lượng dây, cao độ neo dầm, vị trí trên dầm biên dọc, tọa độ neo tháp và độ thẳng đứng của dây treo Golden Gate. Điểm neo tháp của Hiếu có mobility=0, điểm neo dầm có mobility=1. Chế độ cutaway chỉ thay cách trình bày, cùng mặt cắt được áp dụng cho raster, shadow pass và ray picking.
