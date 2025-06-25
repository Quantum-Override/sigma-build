---
name: Enhancement Request
title: "[ENHANCEMENT] <Existing Feature to Improve>"
labels: "enhancement"
assignees: "@yourusername"
---

### **1. Current Limitation**  
<What's suboptimal? Example:>  
"`resolve_vars()` doesn’t handle nested `${VAR}` expansions."

### **2. Proposed Improvement**  
<Specific change. Example:>  
"Support `${VAR:-default}` syntax for fallback values."

### **3. Technical Impact**  
- **Files to Modify**: `var_table.c`, `loader.c`  
- **Backward Compatible**: Yes/No  
