module instances;
import coll;

/* It places, it does not generate: generation follows use; `instance` decides
   where the bodies of non-inline `pub` functions live (§4.9). */
instance coll.stack i32;
