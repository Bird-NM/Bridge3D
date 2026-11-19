# Đồ họa và ảnh đối chiếu

Renderer dùng OpenGL 3.3 Core, VAO/VBO/EBO và các shader trong `shaders/`. Các vật liệu dùng màu, roughness và metallic theo nhóm cấu kiện, ánh sáng trực tiếp GGX cùng ánh sáng môi trường bán cầu. Tính toán ánh sáng trong linear space và lưu màu HDR vào RGBA16F. Sau khi resolve MSAA trong linear space, pass tone mapping nén dải sáng và mã hóa sRGB một lần. ImGui được vẽ sau pass này. `GL_FRAMEBUFFER_SRGB` được tắt để giữ cách mã hóa này nhất quán.

Màu nhựa, bê tông, đá, cỏ và vỏ cây có biến thiên thủ tục trong tọa độ thế giới. Các hàm và nguồn vật liệu nằm cùng mã shader. Vật liệu hiện tại dùng thủ tục, toàn bộ tài nguyên mesh và shader được bàn giao trong repository.

Ánh sáng mặt trời hướng xiên dùng shadow map depth 2048×2048 với PCF 3×3, slope bias và polygon offset. Shadow pass dùng đúng phép biến dạng và ma trận mô hình của color pass. Shadow map được giữ lại khi chỉ xoay camera, cập nhật khi cầu, trạng thái hiển thị hoặc tải thay đổi. Pháp tuyến dùng normal matrix nghịch đảo chuyển vị. Các mesh được kiểm tra winding và bật back-face culling.

Cảnh được vẽ vào framebuffer màu HDR RGBA16F/depth24 MSAA tối đa 4 mẫu rồi resolve sang texture RGBA16F, tone map và xuất sang cửa sổ trước khi chụp ảnh. Kích thước tài nguyên được cấp lại khi cửa sổ thay đổi. Shadow map dùng depth framebuffer riêng và kiểm tra trạng thái hoàn chỉnh khi tạo.

Nước dùng Fresnel, phản xạ màu trời, normal sóng nhẹ thay đổi theo thời gian và phản sáng mặt trời tiết chế. Độ chi tiết gợn được giảm theo footprint pixel ở xa. Water ripples cho phép chuyển sang normal phẳng. Trời dùng gradient theo hướng nhìn và đĩa mặt trời mềm. Sương khoảng cách bắt đầu ở hậu cảnh xa, vùng cầu gần giữ màu và độ tương phản.

## Bố cục

Preset tổng thể dùng FOV 46°, góc 3/4 với yaw 0.45 và pitch 0.18 radian. Fit Bridge dựa vào hộp bao cầu cùng đoạn tiếp cận ngắn. Ở cửa sổ mặc định, cầu chiếm khoảng 70% chiều ngang vùng xem. Fit whole scene đưa cả hai đường dẫn và hai bờ gần vào khung. Góc Road level đặt mắt ở cao độ gần người đi đường. Tower close-up và End A/B dành cho kiểm tra chi tiết.

## Trước và sau ở cùng camera

Hai ảnh trước được lấy từ ứng dụng tại mốc đã sửa tháp Hiếu và thêm cảnh quan cơ bản, trước khi thay pipeline ánh sáng/vật liệu. Cặp sau dùng cùng camera và kích thước vùng xem, có cập nhật hình học chi tiết, cảnh quan, ánh sáng và MSAA.

Cả hai cặp dùng target (0, 27.72, 0), yaw 0.45, pitch 0.18 radian, distance 330, perspective FOV 46°, viewport 1115×836 trong cửa sổ 1600×960. Góc mặc định mới có căn khung gần hơn và được ghi ở ảnh `golden-surface.png`.

| Cầu | Trước | Sau |
| --- | --- | --- |
| Golden Gate | [Trước đồ họa](../captures/golden-before-graphics.png) | [Sau đồ họa](../captures/golden-presentation.png) |
| Sông Hiếu | [Trước đồ họa](../captures/hieu-before-graphics.png) | [Sau đồ họa](../captures/hieu-presentation.png) |

## Ảnh từ bản chạy mới

| Nội dung | Tệp |
| --- | --- |
| Tổng thể với camera mặc định mới | [Golden Gate](../captures/golden-surface.png), [Hiếu](../captures/hieu-surface.png) |
| Cận tháp và mặt đường | [Tháp](../captures/golden-tower-close.png), [Mặt đường](../captures/golden-road-close.png) |
| Bốn đầu cầu | [Golden A](../captures/golden-end-a.png), [Golden B](../captures/golden-end-b.png), [Hiếu A](../captures/hieu-end-a.png), [Hiếu B](../captures/hieu-end-b.png) |
| Khung dây tiếp giáp | [Golden](../captures/golden-interface-wire.png), [Hiếu](../captures/hieu-interface-wire.png) |
| Inspection | [Golden](../captures/golden-inspection.png), [Hiếu](../captures/hieu-inspection.png) |
| So sánh đồng bộ | [Hai viewport](../captures/comparison.png) |
| Minh họa biến dạng | [Golden](../captures/deformation.png), [Hiếu](../captures/hieu-deformation.png) |
| Đối chiếu bóng cùng camera/thời gian nước | [Bật bóng](../captures/golden-presentation.png), [Tắt bóng](../captures/golden-no-shadows.png) |

Ảnh đầu cầu và bố cục được chụp trực tiếp từ framebuffer bằng chức năng xuất PNG. Tất cả ảnh mới được tái tạo bởi `--smoke`, hai ảnh trước được giữ riêng.

## Phạm vi hiện tại

Cây, đá và nhà vẫn là tài nguyên sinh thủ tục với mức chi tiết giới hạn. Ánh sáng môi trường dùng mô hình bán cầu. SSAO, cascaded shadow maps, IBL HDR và planar reflection hiện chưa được triển khai. Hiệu năng GPU và DPI trên Windows đang chờ kiểm tra thực tế. Shadow map đơn có giới hạn độ phân giải ở các chi tiết rất nhỏ và vùng xa. Số đo trên renderer CPU được ghi trong `validation.md`.
