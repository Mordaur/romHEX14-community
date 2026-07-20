# View Sync

View sync keeps multiple open projects lined up as you scroll, zoom and switch views — invaluable when comparing a stock ROM against a modified one, or a base file against a linked ROM.

## Enabling view sync

Toggle **Sync cursors** from the **Selection** menu (its first entry) or the **Sync cursors** button on the Project toolbar (the ⇔ icon). Its tooltip reads "Sync 2D view scroll across all open projects".

romHEX 14 also turns Sync cursors on automatically when you start a comparison — using **Compare Projects…**, opening a linked ROM, importing a linked version, or comparing in the Differences panel. When it does, a brief tooltip appears under the toolbar button reminding you that "Cursors are now synchronized. Click this button to unlink them." The toggle state is remembered between sessions.

## Sync modes

When Sync cursors is on, romHEX 14 links the following across **all open projects**:

- **2D scroll position** — scrolling one waveform scrolls the others to the matching address.
- **2D zoom** — the zoom slider is shared.
- **Hex scroll position** — the Text views stay aligned.
- **Active view tab** — switching one project to Text/2d/3d switches the others too.

If you have set an **alignment** between two ROMs (via the Differences panel), sync uses it: each view scrolls to its own offset-adjusted address, so two ROMs that live at different base addresses still line up physically. Without an alignment, sync matches addresses directly.

Within a single project, switching between the **Text** and **2d** tabs carries your position across automatically. In the **3d** view, the surface crosshair and the mini heatmap cursor stay in sync in both directions — click or drag on either, or use the arrow keys, and both move together.

## Comparing two ROMs side by side

1. Open both projects (for example a stock ROM and your modified version).
2. Choose **Window ▸ Compare Projects…** (also on the **Selection** menu). romHEX 14 tiles the two most-recent project windows left and right and enables Sync cursors so they scroll together. (You need at least two projects open.)
3. Open the same map or region in both — as you scroll or edit one, the other follows.

For a precise byte-by-byte comparison, open the **Differences** panel (**View ▸ Differences**, `Ctrl+D`):

- Pick projects in the **A** and **B** selectors.
- The table lists each differing **Address** with the **A** value, **B** value and **Δ**.
- Use **B offset** / alignment nudges (and **Reset align**) to line up ROMs that sit at different addresses; the alignment then feeds the cursor sync.
- Choose a **Target** and use **Copy selected → Target** or **Copy all → Target** to transfer differences from one ROM into the other.

To compare against the project's own baseline instead of another project, use **View ▸ Differences vs Original** (`Ctrl+Shift+O`), which highlights every changed cell in the hex, 2D and 3D views at once.

!!! note
    **Compare ROM / Version…** and **Compare Hex…** on the Project menu are Pro-build features. The **Sync cursors** toggle, **Compare Projects…** and the **Differences** panel are available in every edition.
