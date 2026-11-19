# Hai bờ và đường tiếp cận

Mỗi cầu có một tệp cấu hình riêng gồm `config/golden-environment.cfg` và `config/hieu-environment.cfg`. Các kích thước, đường bờ, cao độ và mật độ đều là giả định từ hình tham chiếu. Sửa tệp rồi khởi động lại ứng dụng để sinh lại mesh.

`EnvironmentModel` dùng lại Mesh, Component và GpuMesh của ứng dụng. Các nhóm cảnh quan có ID từ 100, tách khỏi các nhóm cầu. Shader giữ cảnh quan cố định khi mô phỏng hoặc explode. Mesh chỉ được tạo lúc khởi động và đổi cầu. Cây dùng các mẫu mesh chung, ghép vào batch tĩnh cho tán và thân.

Hai `RoadConnection` lấy vị trí tâm, hướng ±X, cao độ mặt đường và bề rộng từ chính mô hình cầu. Đường dẫn bắt đầu tại X=±length/2, Y=deckHeight+0.6, Z=0. Đoạn đầu thẳng, sau đó uốn theo đường tâm tham số có tiếp tuyến liên tục. Mặt đường, lề nâng, lan can và vạch sơn lấy chung đường tâm. Tiết diện ngang dùng tiếp tuyến giải tích để tránh xoắn đột ngột tại đầu đường.

## Thông số

| Thông số | Tác động |
| --- | --- |
| approachLength | Chiều dài đường tiếp cận |
| roadCurve, roadDrop | Độ uốn ngang và hạ cao độ về phía đất liền |
| terrainExtension, bankHalfWidth | Phạm vi địa hình hậu cảnh |
| resolution | Độ chia vùng gần cầu, có lưới thưa hơn ở xa |
| hillHeight | Biên độ các đồi, lớn ở Golden Gate và thấp ở Hiếu |
| shoreRun, coastVariation | Chiều rộng mái bờ và độ biến thiên đường bờ |
| roadShoulder | Vùng san nền trước khi chuyển sang đồi |
| promenadeWidth | Bề rộng đường dạo phía Hiếu |
| plantingDepth, treeCount, treeSpacing | Vùng cây, số lượng mục tiêu và khoảng cách tối thiểu |
| treeHeight, treeRadius | Tỷ lệ cây theo đơn vị mô hình |
| buildingStart, buildingEnd, buildingSetback, buildingCount | Vùng và số lượng khối nhà mục tiêu |
| waterR/G/B | Màu nước theo môi trường |
| seed | Tái tạo cùng bố cục cây và đá |

Địa hình dùng các đồi Gaussian, hàm bờ và vùng san nền theo đường. Phía xa hạ dần về dưới mặt nước theo biên elip, tạo đường viền tự nhiên. Các điểm lưới dày ở vùng đường/bờ và thưa ở phía xa. Vật liệu chuyển cỏ–đất–đá theo độ dốc. Golden Gate thêm cụm đá chìm một phần trong mái bờ, điểm ngắm cảnh có đường nối và lan can.

Hiếu có mái kè, đường dạo đi theo đường bờ biến thiên, cây lùi sau đường và mạng đường chữ T đơn giản ở cuối đường dẫn. Đường dạo dừng hai bên mố. Lan can đường dẫn nhường khoảng trống tại nút giao. Các khối nhà có tỷ lệ tầng, mái dốc, gable, bệ và cửa. Bố trí nhà tránh nút giao, cây tránh đường và vùng nhà dự kiến.

## Quan sát và chỉnh sửa

Mở Environment trong bảng trái để bật/tắt từng nhóm, tắt toàn bộ cảnh quan, chỉnh Fade landscape và Water ripples. Khi xem khung dây, bật Landscape wireframe để hiện cạnh của địa hình, đường và kè. Tán cây, nhà và nước giữ chế độ mặt để giảm mật độ cạnh. Mố và khối neo vẫn thuộc nhóm Foundations and anchors của cầu.

Fit whole scene tính phần hai bờ và đường dẫn gần. Mặt nước rộng và địa hình nền phía xa được loại khỏi phép căn khung. End A/B đưa camera đến vùng tiếp giáp tương ứng. Trong so sánh, cả hai phía dùng cùng trạng thái nhóm và camera, phía khung dây mặc định ẩn cảnh quan.

Số cây/nhà thực tế có thể thấp hơn mục tiêu khi các vùng cấm và khoảng cách tối thiểu chiếm chỗ. Tăng mật độ bằng treeCount hoặc giảm treeSpacing trong phạm vi hợp lý. Mức biến thiên địa hình cao cần kiểm tra lại bằng góc dọc đường, từ trên và bộ kiểm tra mesh.

Bài kiểm tra xác nhận đầu đường khớp vị trí, hướng và chiều rộng cầu, kiểm tra cao độ mặt trên đường/lề với tam giác địa hình thực tế, và xác nhận cây/nhà nằm ngoài vùng nhịp–dây cầu. Chạy lại CTest sau khi chỉnh các thông số.
