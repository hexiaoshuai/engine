// Copyright 2017 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/flow/layers/layer_builder.h"

#include "flutter/flow/layers/backdrop_filter_layer.h"
#include "flutter/flow/layers/clip_path_layer.h"
#include "flutter/flow/layers/clip_rect_layer.h"
#include "flutter/flow/layers/clip_rrect_layer.h"
#include "flutter/flow/layers/color_filter_layer.h"
#include "flutter/flow/layers/container_layer.h"
#include "flutter/flow/layers/layer.h"
#include "flutter/flow/layers/layer_tree.h"
#include "flutter/flow/layers/opacity_layer.h"
#include "flutter/flow/layers/performance_overlay_layer.h"
#include "flutter/flow/layers/physical_model_layer.h"
#include "flutter/flow/layers/picture_layer.h"
#include "flutter/flow/layers/shader_mask_layer.h"
#include "flutter/flow/layers/transform_layer.h"

#if defined(OS_FUCHSIA)
#include "flutter/flow/layers/child_scene_layer.h"
#endif  // defined(OS_FUCHSIA)

namespace flow {

LayerBuilder::LayerBuilder() {
  cull_rects_.push(SkRect::MakeLargest());
}

LayerBuilder::~LayerBuilder() = default;

void LayerBuilder::PushTransform(const SkMatrix& sk_matrix) {
  SkMatrix inverse_sk_matrix;
  SkRect cullRect;
  if (sk_matrix.invert(&inverse_sk_matrix)) {
    inverse_sk_matrix.mapRect(&cullRect, cull_rects_.top());
  } else {
    cullRect = SkRect::MakeLargest();
  }

  auto layer = std::make_unique<flow::TransformLayer>();
  layer->set_transform(sk_matrix);
  PushLayer(std::move(layer), cullRect);
}

void LayerBuilder::PushClipRect(const SkRect& clipRect) {
  SkRect cullRect;
  if (!cullRect.intersect(clipRect, cull_rects_.top())) {
    cullRect = SkRect::MakeEmpty();
  }
  auto layer = std::make_unique<flow::ClipRectLayer>();
  layer->set_clip_rect(clipRect);
  PushLayer(std::move(layer), cullRect);
}

void LayerBuilder::PushClipRoundedRect(const SkRRect& rrect) {
  SkRect cullRect;
  if (!cullRect.intersect(rrect.rect(), cull_rects_.top())) {
    cullRect = SkRect::MakeEmpty();
  }
  auto layer = std::make_unique<flow::ClipRRectLayer>();
  layer->set_clip_rrect(rrect);
  PushLayer(std::move(layer), cullRect);
}

void LayerBuilder::PushClipPath(const SkPath& path) {
  SkRect cullRect;
  if (!cullRect.intersect(path.getBounds(), cull_rects_.top())) {
    cullRect = SkRect::MakeEmpty();
  }
  auto layer = std::make_unique<flow::ClipPathLayer>();
  layer->set_clip_path(path);
  PushLayer(std::move(layer), cullRect);
}

void LayerBuilder::PushOpacity(int alpha) {
  auto layer = std::make_unique<flow::OpacityLayer>();
  layer->set_alpha(alpha);
  PushLayer(std::move(layer), cull_rects_.top());
}

void LayerBuilder::PushColorFilter(SkColor color, SkBlendMode blend_mode) {
  auto layer = std::make_unique<flow::ColorFilterLayer>();
  layer->set_color(color);
  layer->set_blend_mode(blend_mode);
  PushLayer(std::move(layer), cull_rects_.top());
}

void LayerBuilder::PushBackdropFilter(sk_sp<SkImageFilter> filter) {
  auto layer = std::make_unique<flow::BackdropFilterLayer>();
  layer->set_filter(filter);
  PushLayer(std::move(layer), cull_rects_.top());
}

void LayerBuilder::PushShaderMask(sk_sp<SkShader> shader,
                                  const SkRect& rect,
                                  SkBlendMode blend_mode) {
  auto layer = std::make_unique<flow::ShaderMaskLayer>();
  layer->set_shader(shader);
  layer->set_mask_rect(rect);
  layer->set_blend_mode(blend_mode);
  PushLayer(std::move(layer), cull_rects_.top());
}

void LayerBuilder::PushPhysicalModel(const SkRRect& sk_rrect,
                                     double elevation,
                                     SkColor color,
                                     SkScalar device_pixel_ratio) {
  SkRect cullRect;
  if (!cullRect.intersect(sk_rrect.rect(), cull_rects_.top())) {
    cullRect = SkRect::MakeEmpty();
  }
  auto layer = std::make_unique<flow::PhysicalModelLayer>();
  layer->set_rrect(sk_rrect);
  layer->set_elevation(elevation);
  layer->set_color(color);
  layer->set_device_pixel_ratio(device_pixel_ratio);
  PushLayer(std::move(layer), cullRect);
}

void LayerBuilder::PushPerformanceOverlay(uint64_t enabled_options,
                                          const SkRect& rect) {
  if (!current_layer_) {
    return;
  }
  auto layer = std::make_unique<flow::PerformanceOverlayLayer>(enabled_options);
  layer->set_paint_bounds(rect);
  current_layer_->Add(std::move(layer));
}

void LayerBuilder::PushPicture(const SkPoint& offset,
                               sk_sp<SkPicture> picture,
                               bool picture_is_complex,
                               bool picture_will_change) {
  if (!current_layer_) {
    return;
  }
  SkRect pictureRect = picture->cullRect();
  pictureRect.offset(offset.x(), offset.y());
  if (!SkRect::Intersects(pictureRect, cull_rects_.top())) {
    return;
  }
  auto layer = std::make_unique<flow::PictureLayer>();
  layer->set_offset(offset);
  layer->set_picture(picture);
  layer->set_is_complex(picture_is_complex);
  layer->set_will_change(picture_will_change);
  current_layer_->Add(std::move(layer));
}

#if defined(OS_FUCHSIA)
void LayerBuilder::PushChildScene(
    const SkPoint& offset,
    const SkSize& size,
    fxl::RefPtr<flow::ExportNodeHolder> export_token_holder,
    bool hit_testable) {
  if (!current_layer_) {
    return;
  }
  SkRect sceneRect =
      SkRect::MakeXYWH(offset.x(), offset.y(), size.width(), size.height());
  if (!SkRect::Intersects(sceneRect, cull_rects_.top())) {
    return;
  }
  auto layer = std::make_unique<flow::ChildSceneLayer>();
  layer->set_offset(offset);
  layer->set_size(size);
  layer->set_export_node_holder(std::move(export_token_holder));
  layer->set_hit_testable(hit_testable);
  current_layer_->Add(std::move(layer));
}
#endif  // defined(OS_FUCHSIA)

void LayerBuilder::Pop() {
  if (!current_layer_) {
    return;
  }
  cull_rects_.pop();
  current_layer_ = current_layer_->parent();
}

int LayerBuilder::GetRasterizerTracingThreshold() const {
  return rasterizer_tracing_threshold_;
}

bool LayerBuilder::GetCheckerboardRasterCacheImages() const {
  return checkerboard_raster_cache_images_;
}

bool LayerBuilder::GetCheckerboardOffscreenLayers() const {
  return checkerboard_offscreen_layers_;
}

void LayerBuilder::SetRasterizerTracingThreshold(uint32_t frameInterval) {
  rasterizer_tracing_threshold_ = frameInterval;
}

void LayerBuilder::SetCheckerboardRasterCacheImages(bool checkerboard) {
  checkerboard_raster_cache_images_ = checkerboard;
}

void LayerBuilder::SetCheckerboardOffscreenLayers(bool checkerboard) {
  checkerboard_offscreen_layers_ = checkerboard;
}

std::unique_ptr<flow::Layer> LayerBuilder::TakeLayer() {
  return std::move(root_layer_);
}

void LayerBuilder::PushLayer(std::unique_ptr<flow::ContainerLayer> layer,
                             const SkRect& cullRect) {
  FXL_DCHECK(layer);

  cull_rects_.push(cullRect);

  if (!root_layer_) {
    root_layer_ = std::move(layer);
    current_layer_ = root_layer_.get();
    return;
  }

  if (!current_layer_) {
    return;
  }

  flow::ContainerLayer* newLayer = layer.get();
  current_layer_->Add(std::move(layer));
  current_layer_ = newLayer;
}

}  // namespace flow
