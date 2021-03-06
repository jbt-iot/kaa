/**
 * Autogenerated by Avro
 * 
 * DO NOT EDIT DIRECTLY
 */
package org.kaaproject.kaa.server.verifiers.facebook.config.gen;  
@SuppressWarnings("all")
@org.apache.avro.specific.AvroGenerated
public class FacebookAvroConfig extends org.apache.avro.specific.SpecificRecordBase implements org.apache.avro.specific.SpecificRecord {
  public static final org.apache.avro.Schema SCHEMA$ = new org.apache.avro.Schema.Parser().parse("{\"type\":\"record\",\"name\":\"FacebookAvroConfig\",\"namespace\":\"org.kaaproject.kaa.server.verifiers.facebook.config.gen\",\"fields\":[{\"name\":\"app_id\",\"type\":{\"type\":\"string\",\"avro.java.string\":\"String\"},\"displayName\":\"Application id\"},{\"name\":\"app_secret\",\"type\":{\"type\":\"string\",\"avro.java.string\":\"String\"},\"displayName\":\"Application secret\"},{\"name\":\"max_parallel_connections\",\"type\":\"int\",\"displayName\":\"Maximal number of allowed connections per verifier\",\"by_default\":\"5\"}]}");
  public static org.apache.avro.Schema getClassSchema() { return SCHEMA$; }
   private java.lang.String app_id;
   private java.lang.String app_secret;
   private int max_parallel_connections;

  /**
   * Default constructor.  Note that this does not initialize fields
   * to their default values from the schema.  If that is desired then
   * one should use {@link \#newBuilder()}. 
   */
  public FacebookAvroConfig() {}

  /**
   * All-args constructor.
   */
  public FacebookAvroConfig(java.lang.String app_id, java.lang.String app_secret, java.lang.Integer max_parallel_connections) {
    this.app_id = app_id;
    this.app_secret = app_secret;
    this.max_parallel_connections = max_parallel_connections;
  }

  public org.apache.avro.Schema getSchema() { return SCHEMA$; }
  // Used by DatumWriter.  Applications should not call. 
  public java.lang.Object get(int field$) {
    switch (field$) {
    case 0: return app_id;
    case 1: return app_secret;
    case 2: return max_parallel_connections;
    default: throw new org.apache.avro.AvroRuntimeException("Bad index");
    }
  }
  // Used by DatumReader.  Applications should not call. 
  @SuppressWarnings(value="unchecked")
  public void put(int field$, java.lang.Object value$) {
    switch (field$) {
    case 0: app_id = (java.lang.String)value$; break;
    case 1: app_secret = (java.lang.String)value$; break;
    case 2: max_parallel_connections = (java.lang.Integer)value$; break;
    default: throw new org.apache.avro.AvroRuntimeException("Bad index");
    }
  }

  /**
   * Gets the value of the 'app_id' field.
   */
  public java.lang.String getAppId() {
    return app_id;
  }

  /**
   * Sets the value of the 'app_id' field.
   * @param value the value to set.
   */
  public void setAppId(java.lang.String value) {
    this.app_id = value;
  }

  /**
   * Gets the value of the 'app_secret' field.
   */
  public java.lang.String getAppSecret() {
    return app_secret;
  }

  /**
   * Sets the value of the 'app_secret' field.
   * @param value the value to set.
   */
  public void setAppSecret(java.lang.String value) {
    this.app_secret = value;
  }

  /**
   * Gets the value of the 'max_parallel_connections' field.
   */
  public java.lang.Integer getMaxParallelConnections() {
    return max_parallel_connections;
  }

  /**
   * Sets the value of the 'max_parallel_connections' field.
   * @param value the value to set.
   */
  public void setMaxParallelConnections(java.lang.Integer value) {
    this.max_parallel_connections = value;
  }

  /** Creates a new FacebookAvroConfig RecordBuilder */
  public static org.kaaproject.kaa.server.verifiers.facebook.config.gen.FacebookAvroConfig.Builder newBuilder() {
    return new org.kaaproject.kaa.server.verifiers.facebook.config.gen.FacebookAvroConfig.Builder();
  }
  
  /** Creates a new FacebookAvroConfig RecordBuilder by copying an existing Builder */
  public static org.kaaproject.kaa.server.verifiers.facebook.config.gen.FacebookAvroConfig.Builder newBuilder(org.kaaproject.kaa.server.verifiers.facebook.config.gen.FacebookAvroConfig.Builder other) {
    return new org.kaaproject.kaa.server.verifiers.facebook.config.gen.FacebookAvroConfig.Builder(other);
  }
  
  /** Creates a new FacebookAvroConfig RecordBuilder by copying an existing FacebookAvroConfig instance */
  public static org.kaaproject.kaa.server.verifiers.facebook.config.gen.FacebookAvroConfig.Builder newBuilder(org.kaaproject.kaa.server.verifiers.facebook.config.gen.FacebookAvroConfig other) {
    return new org.kaaproject.kaa.server.verifiers.facebook.config.gen.FacebookAvroConfig.Builder(other);
  }
  
  /**
   * RecordBuilder for FacebookAvroConfig instances.
   */
  public static class Builder extends org.apache.avro.specific.SpecificRecordBuilderBase<FacebookAvroConfig>
    implements org.apache.avro.data.RecordBuilder<FacebookAvroConfig> {

    private java.lang.String app_id;
    private java.lang.String app_secret;
    private int max_parallel_connections;

    /** Creates a new Builder */
    private Builder() {
      super(org.kaaproject.kaa.server.verifiers.facebook.config.gen.FacebookAvroConfig.SCHEMA$);
    }
    
    /** Creates a Builder by copying an existing Builder */
    private Builder(org.kaaproject.kaa.server.verifiers.facebook.config.gen.FacebookAvroConfig.Builder other) {
      super(other);
      if (isValidValue(fields()[0], other.app_id)) {
        this.app_id = data().deepCopy(fields()[0].schema(), other.app_id);
        fieldSetFlags()[0] = true;
      }
      if (isValidValue(fields()[1], other.app_secret)) {
        this.app_secret = data().deepCopy(fields()[1].schema(), other.app_secret);
        fieldSetFlags()[1] = true;
      }
      if (isValidValue(fields()[2], other.max_parallel_connections)) {
        this.max_parallel_connections = data().deepCopy(fields()[2].schema(), other.max_parallel_connections);
        fieldSetFlags()[2] = true;
      }
    }
    
    /** Creates a Builder by copying an existing FacebookAvroConfig instance */
    private Builder(org.kaaproject.kaa.server.verifiers.facebook.config.gen.FacebookAvroConfig other) {
            super(org.kaaproject.kaa.server.verifiers.facebook.config.gen.FacebookAvroConfig.SCHEMA$);
      if (isValidValue(fields()[0], other.app_id)) {
        this.app_id = data().deepCopy(fields()[0].schema(), other.app_id);
        fieldSetFlags()[0] = true;
      }
      if (isValidValue(fields()[1], other.app_secret)) {
        this.app_secret = data().deepCopy(fields()[1].schema(), other.app_secret);
        fieldSetFlags()[1] = true;
      }
      if (isValidValue(fields()[2], other.max_parallel_connections)) {
        this.max_parallel_connections = data().deepCopy(fields()[2].schema(), other.max_parallel_connections);
        fieldSetFlags()[2] = true;
      }
    }

    /** Gets the value of the 'app_id' field */
    public java.lang.String getAppId() {
      return app_id;
    }
    
    /** Sets the value of the 'app_id' field */
    public org.kaaproject.kaa.server.verifiers.facebook.config.gen.FacebookAvroConfig.Builder setAppId(java.lang.String value) {
      validate(fields()[0], value);
      this.app_id = value;
      fieldSetFlags()[0] = true;
      return this; 
    }
    
    /** Checks whether the 'app_id' field has been set */
    public boolean hasAppId() {
      return fieldSetFlags()[0];
    }
    
    /** Clears the value of the 'app_id' field */
    public org.kaaproject.kaa.server.verifiers.facebook.config.gen.FacebookAvroConfig.Builder clearAppId() {
      app_id = null;
      fieldSetFlags()[0] = false;
      return this;
    }

    /** Gets the value of the 'app_secret' field */
    public java.lang.String getAppSecret() {
      return app_secret;
    }
    
    /** Sets the value of the 'app_secret' field */
    public org.kaaproject.kaa.server.verifiers.facebook.config.gen.FacebookAvroConfig.Builder setAppSecret(java.lang.String value) {
      validate(fields()[1], value);
      this.app_secret = value;
      fieldSetFlags()[1] = true;
      return this; 
    }
    
    /** Checks whether the 'app_secret' field has been set */
    public boolean hasAppSecret() {
      return fieldSetFlags()[1];
    }
    
    /** Clears the value of the 'app_secret' field */
    public org.kaaproject.kaa.server.verifiers.facebook.config.gen.FacebookAvroConfig.Builder clearAppSecret() {
      app_secret = null;
      fieldSetFlags()[1] = false;
      return this;
    }

    /** Gets the value of the 'max_parallel_connections' field */
    public java.lang.Integer getMaxParallelConnections() {
      return max_parallel_connections;
    }
    
    /** Sets the value of the 'max_parallel_connections' field */
    public org.kaaproject.kaa.server.verifiers.facebook.config.gen.FacebookAvroConfig.Builder setMaxParallelConnections(int value) {
      validate(fields()[2], value);
      this.max_parallel_connections = value;
      fieldSetFlags()[2] = true;
      return this; 
    }
    
    /** Checks whether the 'max_parallel_connections' field has been set */
    public boolean hasMaxParallelConnections() {
      return fieldSetFlags()[2];
    }
    
    /** Clears the value of the 'max_parallel_connections' field */
    public org.kaaproject.kaa.server.verifiers.facebook.config.gen.FacebookAvroConfig.Builder clearMaxParallelConnections() {
      fieldSetFlags()[2] = false;
      return this;
    }

    @Override
    public FacebookAvroConfig build() {
      try {
        FacebookAvroConfig record = new FacebookAvroConfig();
        record.app_id = fieldSetFlags()[0] ? this.app_id : (java.lang.String) defaultValue(fields()[0]);
        record.app_secret = fieldSetFlags()[1] ? this.app_secret : (java.lang.String) defaultValue(fields()[1]);
        record.max_parallel_connections = fieldSetFlags()[2] ? this.max_parallel_connections : (java.lang.Integer) defaultValue(fields()[2]);
        return record;
      } catch (Exception e) {
        throw new org.apache.avro.AvroRuntimeException(e);
      }
    }
  }
}
