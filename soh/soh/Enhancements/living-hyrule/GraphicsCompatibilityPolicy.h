#pragma once

namespace LivingHyrule {

// Facts about one skeleton, limb, animation or collision resource, not the
// global graphics-pack preference. Texture replacements do not appear here.
struct NativeGraphicsResource {
    bool present = false;
    bool fromGameArchive = false;
    bool metadataOverride = false;
    bool alternatePresent = false;
};

template <typename Resources, typename Lookup>
bool NativeEncounterGraphicsAvailable(const Resources& required, bool alternatives, Lookup lookup) {
    for (const auto& name : required) {
        const auto resource = lookup(name);
        if (!resource.present || !resource.fromGameArchive || resource.metadataOverride ||
            (alternatives && resource.alternatePresent))
            return false;
    }
    return true;
}

} // namespace LivingHyrule
