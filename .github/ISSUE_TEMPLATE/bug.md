---
name: Bug Report
title: "[BUG] <Brief Header>"
labels: "bug"
assignees: "@yourusername"
---

### **1. Issue**    
<What's broken? Example:>
"Memory leak when disposing targets with unresolved dependencies."

### **2. Expected Behavior**  
<What should happen? Example:>  
"All resources should be free when calling `Resources.dispose_target()`."

### **3. Reproduction Steps**
<How to reproduce the issue. Example:>  
``` bash
./sbuild --build broken.json:leaky_target --log=2  
```  

### **4. Context**
- Affected Files:  
  - <file>
  - <file>
- Error Log: [Link/Paste Valgrind output]
