//------------------------------------------------------------------------------
// <auto-generated />
//
// This file was automatically generated by SWIG (http://www.swig.org).
// Version 3.0.5
//
// Do not make changes to this file unless you know what you are doing--modify
// the SWIG interface file instead.
//------------------------------------------------------------------------------


public class OpalProductDescription : global::System.IDisposable {
  private global::System.Runtime.InteropServices.HandleRef swigCPtr;
  protected bool swigCMemOwn;

  internal OpalProductDescription(global::System.IntPtr cPtr, bool cMemoryOwn) {
    swigCMemOwn = cMemoryOwn;
    swigCPtr = new global::System.Runtime.InteropServices.HandleRef(this, cPtr);
  }

  internal static global::System.Runtime.InteropServices.HandleRef getCPtr(OpalProductDescription obj) {
    return (obj == null) ? new global::System.Runtime.InteropServices.HandleRef(null, global::System.IntPtr.Zero) : obj.swigCPtr;
  }

  ~OpalProductDescription() {
    Dispose();
  }

  public virtual void Dispose() {
    lock(this) {
      if (swigCPtr.Handle != global::System.IntPtr.Zero) {
        if (swigCMemOwn) {
          swigCMemOwn = false;
          OPALPINVOKE.delete_OpalProductDescription(swigCPtr);
        }
        swigCPtr = new global::System.Runtime.InteropServices.HandleRef(null, global::System.IntPtr.Zero);
      }
      global::System.GC.SuppressFinalize(this);
    }
  }

  public string vendor {
    set {
      OPALPINVOKE.OpalProductDescription_vendor_set(swigCPtr, value);
    } 
    get {
      string ret = OPALPINVOKE.OpalProductDescription_vendor_get(swigCPtr);
      return ret;
    } 
  }

  public string name {
    set {
      OPALPINVOKE.OpalProductDescription_name_set(swigCPtr, value);
    } 
    get {
      string ret = OPALPINVOKE.OpalProductDescription_name_get(swigCPtr);
      return ret;
    } 
  }

  public string version {
    set {
      OPALPINVOKE.OpalProductDescription_version_set(swigCPtr, value);
    } 
    get {
      string ret = OPALPINVOKE.OpalProductDescription_version_get(swigCPtr);
      return ret;
    } 
  }

  public uint t35CountryCode {
    set {
      OPALPINVOKE.OpalProductDescription_t35CountryCode_set(swigCPtr, value);
    } 
    get {
      uint ret = OPALPINVOKE.OpalProductDescription_t35CountryCode_get(swigCPtr);
      return ret;
    } 
  }

  public uint t35Extension {
    set {
      OPALPINVOKE.OpalProductDescription_t35Extension_set(swigCPtr, value);
    } 
    get {
      uint ret = OPALPINVOKE.OpalProductDescription_t35Extension_get(swigCPtr);
      return ret;
    } 
  }

  public uint manufacturerCode {
    set {
      OPALPINVOKE.OpalProductDescription_manufacturerCode_set(swigCPtr, value);
    } 
    get {
      uint ret = OPALPINVOKE.OpalProductDescription_manufacturerCode_get(swigCPtr);
      return ret;
    } 
  }

  public OpalProductDescription() : this(OPALPINVOKE.new_OpalProductDescription(), true) {
  }

}
