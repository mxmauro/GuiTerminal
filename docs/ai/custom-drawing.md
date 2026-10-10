# Draw a custom Direct2D region

A custom-draw region occupies a cell-defined rectangle in the region hierarchy, but its callback
receives a `DrawContext` in device-independent pixels (DIPs). Use it for charts, gauges, and other
graphics that are awkward to represent as terminal cells.

## Create and invalidate a custom region

```cpp
static VOID DrawChart(GuiTerminal::DrawContext &drawContext, GuiTerminal::RegionHandle hRegion) noexcept
{
    const FLOAT fWidth = static_cast<FLOAT>(drawContext.GetWidth());
    const FLOAT fHeight = static_cast<FLOAT>(drawContext.GetHeight());

    (void)hRegion;
    drawContext.Clear(RGB(20U, 38U, 66U));
    drawContext.DrawRectangle(D2D1::RectF(4.0f, 4.0f, fWidth - 4.0f, fHeight - 4.0f), RGB(130U, 220U, 255U), 2.0f);
    drawContext.FillEllipse(D2D1::Ellipse(D2D1::Point2F(fWidth * 0.5f, fHeight * 0.5f), 18.0f, 18.0f), RGB(102U, 230U, 172U));
}

GuiTerminal::RegionHandle chartRegion = nullptr;
HRESULT hr = terminal->CreateCustomDrawRegion(52, 3, 42, 16, &chartRegion, DrawChart);
if (FAILED(hr))
{
    return hr;
}

terminal->InvalidateRegion(chartRegion);
```

The region geometry is in cells; `DrawContext` sizes and primitive coordinates are in DIPs.
`InvalidateRegion(chartRegion)` requests a new draw for that region. A normal child region can add
terminal text over the graphic.

`DrawContext` provides clear, point, line, rectangle, rounded-rectangle, ellipse, styled-text, and
path operations. Build a path with `BeginPath`, segment methods, and optionally `ClosePath`, then
call `StrokePath` or `FillPath`.

## Own Direct2D resources in callback state

Capture shared state in both callbacks so it outlives the region. `std::shared_ptr` protects the
state object's lifetime, not concurrent access to its fields. Separate `shared_ptr` instances can
update their reference counts safely, but neither the shared object nor one `shared_ptr` variable is
made safe for concurrent access.

```cpp
typedef struct ChartDrawState_s {
    std::mutex modelMutex;
    FLOAT fValue{};
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> accentBrush;
} ChartDrawState;

const std::shared_ptr<ChartDrawState> drawState = std::make_shared<ChartDrawState>();
```

During a draw callback, `GetDirect2DRenderTarget` is valid only for that callback. Do not store the
returned pointer. It is normally available during a callback, but code that uses it should still
handle `nullptr` by skipping its advanced Direct2D work. Create a callback-owned resource lazily
when it is null:

```cpp
ID2D1RenderTarget *renderTarget = drawContext.GetDirect2DRenderTarget();
if (renderTarget && !drawState->accentBrush)
{
    (void)renderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Aqua),
                                               drawState->accentBrush.GetAddressOf());
}
```

Register `SetCustomDrawRegionResourceCleanup` and reset every render-target-dependent resource in
its callback. Cleanup runs before target loss, target resize, DPI changes, and region destruction;
it may run more than once before another draw, so it must be idempotent. The next draw recreates
any null resource and does not need to know why it was released.

GuiTerminal serializes all custom-draw and resource-cleanup callbacks for one control. Therefore, a
resource accessed only by those callbacks needs no application mutex. `DestroyRegion` waits for an
active draw callback to finish, so `RegionDestroyed` cleanup cannot run concurrently with it.

If another thread also accesses a resource member, add a mutex and copy its `ComPtr` under that
mutex before use. The local copy preserves lifetime after the lock is released; copying without the
mutex races with `Reset()`.

```cpp
typedef struct SharedBrushState_s {
    std::mutex resourceMutex;
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> accentBrush;
} SharedBrushState;

SharedBrushState sharedBrushState;
Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> accentBrush;
{
    std::lock_guard<std::mutex> lockGuard(sharedBrushState.resourceMutex);

    accentBrush = sharedBrushState.accentBrush;
}
```

The draw and cleanup callbacks execute while GuiTerminal holds its internal control lock. They must
not call `InvalidateRegion`, `DestroyRegion`, or any other `Control` method. When application code
updates callback-owned model data, release its mutex before calling `InvalidateRegion`; otherwise it
can invert the application lock and GuiTerminal control-lock order.
